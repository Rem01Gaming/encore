import { MMRLInterfaceFactory } from "mmrl";
const mmrl = MMRLInterfaceFactory("encore");

// inject MMRL CSS
mmrl.injectStyleSheets();

// handle no-js-api permission
if (!mmrl.hasAccessToAdvancedKernelSuAPI) {
  try {
    mmrl.requestAdvancedKernelSUAPI();
  } catch (error) {
    mmrl_denied.showModal();
  }
}
