import { MMRLInterfaceFactory } from "mmrl";
const mmrl = MMRLInterfaceFactory("encore");

// inject MMRL CSS
mmrl.injectStyleSheets();

// handle no-js-api permission
if (!mmrl.hasAccessToAdvancedKernelSuAPI) {
    if (!mmrl.requestAdvancedKernelSUAPI()) {
        mmrl_denied.showModal();
    }
}
