import wasm from "./diplomat-wasm.mjs"
import * as diplomatRuntime from "./diplomat-runtime.mjs"
import { ICU4XCollatorAlternateHandling_js_to_rust, ICU4XCollatorAlternateHandling_rust_to_js } from "./ICU4XCollatorAlternateHandling.mjs"
import { ICU4XCollatorBackwardSecondLevel_js_to_rust, ICU4XCollatorBackwardSecondLevel_rust_to_js } from "./ICU4XCollatorBackwardSecondLevel.mjs"
import { ICU4XCollatorCaseFirst_js_to_rust, ICU4XCollatorCaseFirst_rust_to_js } from "./ICU4XCollatorCaseFirst.mjs"
import { ICU4XCollatorCaseLevel_js_to_rust, ICU4XCollatorCaseLevel_rust_to_js } from "./ICU4XCollatorCaseLevel.mjs"
import { ICU4XCollatorMaxVariable_js_to_rust, ICU4XCollatorMaxVariable_rust_to_js } from "./ICU4XCollatorMaxVariable.mjs"
import { ICU4XCollatorNumeric_js_to_rust, ICU4XCollatorNumeric_rust_to_js } from "./ICU4XCollatorNumeric.mjs"
import { ICU4XCollatorStrength_js_to_rust, ICU4XCollatorStrength_rust_to_js } from "./ICU4XCollatorStrength.mjs"

export class ICU4XCollatorResolvedOptionsV1 {
  constructor(underlying) {
    this.strength = ICU4XCollatorStrength_rust_to_js[diplomatRuntime.enumDiscriminant(wasm, underlying)];
    this.alternate_handling = ICU4XCollatorAlternateHandling_rust_to_js[diplomatRuntime.enumDiscriminant(wasm, underlying + 4)];
    this.case_first = ICU4XCollatorCaseFirst_rust_to_js[diplomatRuntime.enumDiscriminant(wasm, underlying + 8)];
    this.max_variable = ICU4XCollatorMaxVariable_rust_to_js[diplomatRuntime.enumDiscriminant(wasm, underlying + 12)];
    this.case_level = ICU4XCollatorCaseLevel_rust_to_js[diplomatRuntime.enumDiscriminant(wasm, underlying + 16)];
    this.numeric = ICU4XCollatorNumeric_rust_to_js[diplomatRuntime.enumDiscriminant(wasm, underlying + 20)];
    this.backward_second_level = ICU4XCollatorBackwardSecondLevel_rust_to_js[diplomatRuntime.enumDiscriminant(wasm, underlying + 24)];
  }
}
