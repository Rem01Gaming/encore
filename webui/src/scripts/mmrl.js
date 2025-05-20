import { MMRLInterfaceFactory } from "mmrl";
const mmrl = MMRLInterfaceFactory("encore");

// handle no-js-api permission
if (!mmrl.hasAccessToAdvancedKernelSuAPI) {
    if (!mmrl.requestAdvancedKernelSUAPI()) {
        mmrl_denied.showModal();
    }
}
