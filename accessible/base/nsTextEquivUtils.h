/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim:expandtab:shiftwidth=2:tabstop=2:
 */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef _nsTextEquivUtils_H_
#define _nsTextEquivUtils_H_

#include "mozilla/a11y/Accessible.h"
#include "mozilla/a11y/Role.h"

class nsIContent;

namespace mozilla {
namespace a11y {
class LocalAccessible;
}
}  // namespace mozilla

/**
 * Text equivalent computation rules. These rules are mapped to accessible roles
 * in RoleMap.h.
 */
enum ETextEquivRule {
  // No rule. Equivalent to "name from author."
  eNoNameRule = 0x00,

  // Walk into subtree only if the currently navigated accessible is not root
  // accessible (i.e. if the accessible is part of text equivalent computation).
  eNameFromSubtreeIfReqRule = 0x01,

  // Text equivalent computation from subtree is allowed. Equivalent to "name
  // from content."
  eNameFromSubtreeRule = 0x03,

  // The accessible allows to append its value to text equivalent.
  // XXX: This is temporary solution. Once we move accessible value of links
  // and linkable accessibles to MSAA part we can remove this.
  eNameFromValueRule = 0x04
};

/**
 * The class provides utils methods to compute the accessible name and
 * description. Note that, as of the Accessible Name and Description Computation
 * 1.2 specification, the phrases "text equivalent" and "text alternative" are
 * used interchangably.
 */
class nsTextEquivUtils {
 public:
  typedef mozilla::a11y::LocalAccessible LocalAccessible;
  typedef mozilla::a11y::Accessible Accessible;

  /**
   * Determines if the accessible has a given name rule.
   *
   * @param aAccessible [in] the given accessible
   * @param aRule       [in] a given name rule
   * @return true if the accessible has the rule
   */
  static inline bool HasNameRule(Accessible* aAccessible,
                                 ETextEquivRule aRule) {
    return (GetRoleRule(aAccessible->Role()) & aRule) == aRule;
  }

  /**
   * Calculates the name from the given accessible's subtree, if allowed.
   *
   * @param aAccessible [in] the given accessible
   * @param aName       [out] accessible name
   */
  static nsresult GetNameFromSubtree(const LocalAccessible* aAccessible,
                                     nsAString& aName);

  /**
   * Calculates text equivalent from the subtree. This function is similar to
   * GetNameFromSubtree, but it returns a non-empty result for things like
   * HTML:p, since it does not verify that the given accessible allows name
   * from content.
   */
  static void GetTextEquivFromSubtree(const Accessible* aAccessible,
                                      nsString& aTextEquiv) {
    aTextEquiv.Truncate();

    AppendFromAccessibleChildren(aAccessible, &aTextEquiv);
    aTextEquiv.CompressWhitespace();
  }

  /**
   * Calculates the text equivalent for the given accessible from one of its
   * IDRefs attributes (like aria-labelledby or aria-describedby).
   *
   * @param aAccessible  [in] the accessible text equivalent is computed for
   * @param aIDRefsAttr  [in] IDRefs attribute on DOM node of the accessible
   * @param aTextEquiv   [out] result text equivalent
   */
  static nsresult GetTextEquivFromIDRefs(const LocalAccessible* aAccessible,
                                         nsAtom* aIDRefsAttr,
                                         nsAString& aTextEquiv);

  /**
   * Calculates the text equivalent from the given content - and its subtree, if
   * allowed - and appends it to the given string.
   *
   * @param aInitiatorAcc  [in] the accessible text equivalent is computed for
   *                       in the end (root accessible of text equivalent
   *                       calculation recursion)
   * @param aContent       [in] the given content the text equivalent is
   *                       computed from
   * @param aString        [in, out] the string
   */
  static nsresult AppendTextEquivFromContent(
      const LocalAccessible* aInitiatorAcc, nsIContent* aContent,
      nsAString* aString);

  /**
   * Calculates the text equivalent from the given text content (may be text
   * node or html:br) and appends it to the given string.
   *
   * @param aContent       [in] the text content
   * @param aString        [in, out] the string
   */
  static nsresult AppendTextEquivFromTextContent(nsIContent* aContent,
                                                 nsAString* aString);

  /**
   * Iterates DOM children and calculates the text equivalent from each child
   * node. Then, appends the calculated text to the given string.
   *
   * @param aContent      [in] the node to fetch DOM children from
   * @param aString       [in, out] the string
   */
  static nsresult AppendFromDOMChildren(nsIContent* aContent,
                                        nsAString* aString);

 private:
  /**
   * Iterates the given accessible's children and calculates the text equivalent
   * from each child. Then, appends the calculated text to the given string.
   */
  static nsresult AppendFromAccessibleChildren(const Accessible* aAccessible,
                                               nsAString* aString);

  /**
   * Calculates the text equivalent from the given accessible - and its subtree,
   * if allowed. Then, appends the calculated text to the given string.
   */
  static nsresult AppendFromAccessible(Accessible* aAccessible,
                                       nsAString* aString);

  /**
   * Calculates the text equivalent from the value of the given accessible.
   * Then, appends the calculated text to the given string. This function
   * implements the "Embedded Control" section of the AccName spec.
   */
  static nsresult AppendFromValue(Accessible* aAccessible, nsAString* aString);

  /**
   * Calculates the text equivalent from the given DOM node - and its subtree,
   * if allowed. Then, appends the calculated text to the given string.
   */
  static nsresult AppendFromDOMNode(nsIContent* aContent, nsAString* aString);

  /**
   * Concatenates the given strings and appends space between them. Returns true
   * if the text equivalent string was appended.
   */
  static bool AppendString(nsAString* aString,
                           const nsAString& aTextEquivalent);

  /**
   * Returns the rule (constant of ETextEquivRule) for a given role.
   */
  static uint32_t GetRoleRule(mozilla::a11y::roles::Role aRole);

  /**
   * Returns true if a given accessible should be included when calculating
   * the text equivalent for the initiator's subtree.
   */
  static bool ShouldIncludeInSubtreeCalculation(Accessible* aAccessible);

  /**
   * Returns true if the given accessible is a text leaf containing only
   * whitespace.
   */
  static bool IsWhitespaceLeaf(Accessible* aAccessible);
};

#endif
