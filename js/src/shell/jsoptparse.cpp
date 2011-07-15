/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * vim: set ts=8 sw=4 et tw=99:
 *
 * ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is SpiderMonkey JavaScript shell.
 *
 * The Initial Developer of the Original Code is
 * Mozilla Corporation.
 * Portions created by the Initial Developer are Copyright (C) 2010
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Christopher D. Leary <cdleary@mozilla.com>
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either of the GNU General Public License Version 2 or later (the "GPL"),
 * or the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#include "jsoptparse.h"
#include <ctype.h>
#include <stdarg.h>

using namespace js;
using namespace js::cli;
using namespace js::cli::detail;

const char OptionParser::prognameMeta[] = "{progname}";

#define OPTION_CONVERT_IMPL(__cls) \
    bool \
    Option::is##__cls##Option() const \
    { \
        return kind == OptionKind##__cls; \
    } \
    __cls##Option * \
    Option::as##__cls##Option() \
    { \
        JS_ASSERT(is##__cls##Option()); \
        return static_cast<__cls##Option *>(this); \
    } \
    const __cls##Option * \
    Option::as##__cls##Option() const \
    { \
        return const_cast<Option *>(this)->as##__cls##Option(); \
    }

ValuedOption *
Option::asValued()
{
    JS_ASSERT(isValued());
    return static_cast<ValuedOption *>(this);
}

const ValuedOption *
Option::asValued() const
{
    return const_cast<Option *>(this)->asValued();
}

OPTION_CONVERT_IMPL(Bool)
OPTION_CONVERT_IMPL(String)
OPTION_CONVERT_IMPL(Int)
OPTION_CONVERT_IMPL(MultiString)

OptionParser::Result
OptionParser::error(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    fprintf(stderr, "Error: ");
    vfprintf(stderr, fmt, args);
    va_end(args);
    fputs("\n\n", stderr);
    return ParseError;
}

/* Quick and dirty paragraph printer. */
static void
PrintParagraph(const char *text, uintN startColno, const uintN limitColno, bool padFirstLine)
{
    uintN colno = startColno;
    const char *it = text;

    if (padFirstLine)
        printf("%*s", startColno, "");

    while (*it != '\0') {
        JS_ASSERT(!isspace(*it));

        /* Delimit the current token. */
        const char *limit = it;
        while (!isspace(*limit) && *limit != '\0')
            ++limit;

        /* 
         * If the current token is longer than the available number of columns,
         * then make a line break before printing the token.
         */
        JS_ASSERT(limit - it > 0);
        size_t tokLen = limit - it;
        JS_ASSERT(tokLen);
        if (tokLen + colno >= limitColno) {
            printf("\n%*s%.*s", startColno, "", int(tokLen), it);
            colno = startColno + tokLen;
        } else {
            printf("%.*s", int(tokLen), it);
            colno += tokLen;
        }

        switch (*limit) {
          case '\0':
            return;
          case ' ':
            putchar(' ');
            colno += 1;
            it = limit;
            while (*it == ' ')
                ++it;
            break;
          case '\n':
            /* |text| wants to force a newline here. */
            printf("\n%*s", startColno, "");
            colno = startColno;
            it = limit + 1;
            /* Could also have line-leading spaces. */
            while (*it == ' ') {
                putchar(' ');
                ++colno;
                ++it;
            }
            break;
          default:
            JS_NOT_REACHED("unhandled token splitting character in text");
        }
    }
}

OptionParser::Result
OptionParser::printHelp(const char *progname)
{
    const char *prefixEnd = strstr(usage, prognameMeta);
    if (prefixEnd) {
        printf("%.*s%s%s\n", int(prefixEnd - usage), usage, progname,
               prefixEnd + sizeof(prognameMeta) - 1);
    } else {
        puts(usage);
    }

    if (descr) {
        putchar('\n');
        PrintParagraph(descr, 2, descrWidth, true);
        putchar('\n');
    }

    if (ver)
        printf("\nVersion: %s\n\n", ver);

    if (!arguments.empty()) {
        printf("Arguments:\n");

        static const char fmt[] = "  %s ";
        size_t fmtChars = sizeof(fmt) - 2;
        size_t lhsLen = 0;
        for (Option **it = arguments.begin(), **end = arguments.end(); it != end; ++it)
            lhsLen = JS_MAX(lhsLen, strlen((*it)->longflag) + fmtChars);

        for (Option **it = arguments.begin(), **end = arguments.end(); it != end; ++it) {
            Option *arg = *it;
            size_t chars = printf(fmt, arg->longflag);
            for (; chars < lhsLen; ++chars)
                putchar(' ');
            PrintParagraph(arg->help, lhsLen, helpWidth, false);
            putchar('\n');
        }
        putchar('\n');
    }

    if (!options.empty()) {
        printf("Options:\n");

        static const char fmt[] = "  -%c --%s ";
        size_t fmtChars = sizeof(fmt) - 3; /* How many chars w/o longflag? */
        static const char fmtValued[] = "  -%c --%s=%s ";
        size_t fmtValuedChars = sizeof(fmtValued) - 5;

        /* Calculate sizes for column alignment. */
        size_t lhsLen = 0;
        for (Option **it = options.begin(), **end = options.end(); it != end; ++it) {
            Option *opt = *it;
            size_t longflagLen = strlen(opt->longflag);
            size_t len = opt->isValued()
                         ? fmtValuedChars + longflagLen + strlen(opt->asValued()->metavar)
                         : fmtChars + longflagLen;
            lhsLen = JS_MAX(lhsLen, len);
        }

        /* Print option help text. */
        for (Option **it = options.begin(), **end = options.end(); it != end; ++it) {
            Option *opt = *it;
            size_t chars = opt->isValued()
                           ? printf(fmtValued, opt->shortflag, opt->longflag,
                                    opt->asValued()->metavar)
                           : printf(fmt, opt->shortflag, opt->longflag);
            for (; chars < lhsLen; ++chars)
                putchar(' ');
            PrintParagraph(opt->help, lhsLen, helpWidth, false);
            putchar('\n');
        }
    }

    return ParseHelp;
}

OptionParser::Result
OptionParser::extractValue(size_t argc, char **argv, size_t *i, char **value)
{
    JS_ASSERT(*i < argc);
    char *eq = strchr(argv[*i], '=');
    if (eq) {
        *value = eq + 1;
        if (value[0] == '\0')
            return error("A value is required for option %.*s", eq - argv[*i], argv[*i]);
        return Okay;
    }

    if (argc == *i + 1)
        return error("Expected a value for option %s", argv[*i]);

    *i += 1;
    *value = argv[*i];
    return Okay;
}

OptionParser::Result
OptionParser::handleOption(Option *opt, size_t argc, char **argv, size_t *i)
{
    switch (opt->kind) {
      case OptionKindBool:
      {
        if (opt == &helpOption)
            return printHelp(argv[0]);
        opt->asBoolOption()->value = true;
        return Okay;
      }
      /* 
       * Valued options are allowed to specify their values either via
       * successive arguments or a single --longflag=value argument.
       */
      case OptionKindString:
      {
        char *value;
        if (Result r = extractValue(argc, argv, i, &value))
            return r;
        opt->asStringOption()->value = value;
        return Okay;
      }
      case OptionKindInt:
      {
        char *value;
        if (Result r = extractValue(argc, argv, i, &value))
            return r;
        opt->asIntOption()->value = atoi(value);
        return Okay;
      }
      case OptionKindMultiString:
      {
        char *value;
        if (Result r = extractValue(argc, argv, i, &value))
            return r;
        StringArg arg(value, *i);
        return opt->asMultiStringOption()->strings.append(arg) ? Okay : Fail;
      }
      default:
        JS_NOT_REACHED("unhandled option kind");
        return Fail;
    }
}

OptionParser::Result
OptionParser::handleArg(size_t argc, char **argv, size_t *i)
{
    if (nextArgument >= arguments.length())
        return error("Too many arguments provided");

    Option *arg = arguments[nextArgument];
    switch (arg->kind) {
      case OptionKindString:
        arg->asStringOption()->value = argv[*i];
        nextArgument += 1;
        return Okay;
      case OptionKindMultiString:
      {
        /* Don't advance the next argument -- there can only be one (final) variadic argument. */
        StringArg value(argv[*i], *i);
        return arg->asMultiStringOption()->strings.append(value) ? Okay : Fail;
      }
      default:
        JS_NOT_REACHED("unhandled argument kind");
        return Fail;
    }
}

OptionParser::Result
OptionParser::parseArgs(int inputArgc, char **argv)
{
    JS_ASSERT(inputArgc >= 0);
    size_t argc = inputArgc;

    for (size_t i = 1; i < argc; ++i) {
        char *arg = argv[i];
        Result r;
        if (arg[0] == '-') {
            /* Option. */
            size_t arglen = strlen(arg);
            if (arglen < 2) /* Do not permit solo dash option. */
                return error("Invalid dash option");

            Option *opt;
            if (arg[1] == '-') {
                /* Long option. */
                opt = findOption(arg + 2);
                if (!opt)
                    return error("Invalid long option: %s", arg);
            } else {
                /* Short option */
                if (arg[2] != '\0')
                    return error("Short option followed by junk: %s", arg);
                opt = findOption(arg[1]);
                if (!opt)
                    return error("Invalid short option: %s", arg);
            }

            r = handleOption(opt, argc, argv, &i);
        } else {
            /* Argument. */
            r = handleArg(argc, argv, &i);
        }
        switch (r) {
          case Okay:
            break;
          default:
            return r;
        }
    }
    return Okay;
}

void
OptionParser::setHelpOption(char shortflag, const char *longflag, const char *help)
{
    helpOption.setFlagInfo(shortflag, longflag, help);
}

bool
OptionParser::getHelpOption() const
{
    return helpOption.value;
}

bool
OptionParser::getBoolOption(char shortflag) const
{
    return findOption(shortflag)->asBoolOption()->value;
}

int
OptionParser::getIntOption(char shortflag) const
{
    return findOption(shortflag)->asIntOption()->value;
}

const char *
OptionParser::getStringOption(char shortflag) const
{
    return findOption(shortflag)->asStringOption()->value;
}

MultiStringRange
OptionParser::getMultiStringOption(char shortflag) const
{
    const MultiStringOption *mso = findOption(shortflag)->asMultiStringOption();
    return MultiStringRange(mso->strings.begin(), mso->strings.end());
}

OptionParser::~OptionParser()
{
    for (Option **it = options.begin(), **end = options.end(); it != end; ++it)
        Foreground::delete_<Option>(*it);
}

Option *
OptionParser::findOption(char shortflag)
{
    for (Option **it = options.begin(), **end = options.end(); it != end; ++it) {
        if ((*it)->shortflag == shortflag)
            return *it;
    }

    return helpOption.shortflag == shortflag ? &helpOption : NULL;
}

const Option *
OptionParser::findOption(char shortflag) const
{
    return const_cast<OptionParser *>(this)->findOption(shortflag);
}

Option *
OptionParser::findOption(const char *longflag)
{
    for (Option **it = options.begin(), **end = options.end(); it != end; ++it) {
        const char *target = (*it)->longflag;
        if ((*it)->isValued()) {
            size_t targetLen = strlen(target);
            /* Permit a trailing equals sign on the longflag argument. */
            for (size_t i = 0; i < targetLen; ++i) {
                if (longflag[i] == '\0' || longflag[i] != target[i])
                    goto no_match;
            }
            if (longflag[targetLen] == '\0' || longflag[targetLen] == '=')
                return *it;
        } else {
            if (strcmp(target, longflag) == 0)
                return *it;
        }
  no_match:;
    }

    return strcmp(helpOption.longflag, longflag) ? NULL : &helpOption;
}

/* Argument accessors */

Option *
OptionParser::findArgument(const char *name)
{
    for (Option **it = arguments.begin(), **end = arguments.end(); it != end; ++it) {
        const char *target = (*it)->longflag;
        if (strcmp(target, name) == 0)
            return *it;
    }
    return NULL;
}

const Option *
OptionParser::findArgument(const char *name) const
{
    return const_cast<OptionParser *>(this)->findArgument(name);
}

const char *
OptionParser::getStringArg(const char *name) const
{
    return findArgument(name)->asStringOption()->value;
}

MultiStringRange
OptionParser::getMultiStringArg(const char *name) const
{
    const MultiStringOption *mso = findArgument(name)->asMultiStringOption();
    return MultiStringRange(mso->strings.begin(), mso->strings.end());
}

/* Option builders */

bool
OptionParser::addIntOption(char shortflag, const char *longflag, const char *metavar,
                           const char *help, int defaultValue)
{
    if (!options.reserve(options.length() + 1))
        return false;
    IntOption *io = OffTheBooks::new_<IntOption>(shortflag, longflag, help, metavar,
                                                 defaultValue);
    if (!io)
        return false;
    options.infallibleAppend(io);
    return true;
}

bool
OptionParser::addBoolOption(char shortflag, const char *longflag, const char *help)
{
    if (!options.reserve(options.length() + 1))
        return false;
    BoolOption *bo = OffTheBooks::new_<BoolOption>(shortflag, longflag, help);
    if (!bo)
        return false;
    options.infallibleAppend(bo);
    return true;
}

bool
OptionParser::addStringOption(char shortflag, const char *longflag, const char *metavar,
                              const char *help)
{
    if (!options.reserve(options.length() + 1))
        return false;
    StringOption *so = OffTheBooks::new_<StringOption>(shortflag, longflag, help, metavar);
    if (!so)
        return false;
    options.infallibleAppend(so);
    return true;
}

bool
OptionParser::addMultiStringOption(char shortflag, const char *longflag, const char *metavar,
                                   const char *help)
{
    if (!options.reserve(options.length() + 1))
        return false;
    MultiStringOption *mso = OffTheBooks::new_<MultiStringOption>(shortflag, longflag, help,
                                                                  metavar);
    if (!mso)
        return false;
    options.infallibleAppend(mso);
    return true;
}

/* Argument builders */

bool
OptionParser::addOptionalStringArg(const char *name, const char *help)
{
    if (!arguments.reserve(arguments.length() + 1))
        return false;
    StringOption *so = OffTheBooks::new_<StringOption>(1, name, help, (const char *) NULL);
    if (!so)
        return false;
    arguments.infallibleAppend(so);
    return true;
}

bool
OptionParser::addOptionalMultiStringArg(const char *name, const char *help)
{
    JS_ASSERT_IF(!arguments.empty(), !arguments.back()->isVariadic());
    if (!arguments.reserve(arguments.length() + 1))
        return false;
    MultiStringOption *mso = OffTheBooks::new_<MultiStringOption>(1, name, help,
                                                                  (const char *) NULL);
    if (!mso)
        return false;
    arguments.infallibleAppend(mso);
    return true;
}
