import { initSimulator } from "./sim";
import "./editor";
import "./motion_button";
import "./serial_monitor";
import "./resize";
import { files } from "./files";
import { openFile } from "./editor";
import "./compile";

initSimulator();
openFile(Object.keys(files)[0]);
