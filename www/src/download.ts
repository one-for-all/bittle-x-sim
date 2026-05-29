import JSZip from 'jszip';
import { files } from './files';

export function setupDownload() {
    document.getElementById("downloadButton")!.addEventListener("click", async () => {
        const zip = new JSZip();

        for (const [filename, fileEntry] of Object.entries(files)) {
            zip.file(filename, fileEntry.content);
        }

        const content = await zip.generateAsync({ type: 'blob' });
        const url = URL.createObjectURL(content);
        const a = document.createElement('a');
        a.href = url;
        a.download = 'bittle_x_project.zip';
        document.body.appendChild(a);
        a.click();
        document.body.removeChild(a);
        URL.revokeObjectURL(url);
    });
}
