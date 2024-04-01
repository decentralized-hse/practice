import java.io.IOException;
import java.security.NoSuchAlgorithmException;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

public class DirectoryCreator {

    public String makeDir(String path, String parentHash) throws IOException, NoSuchAlgorithmException {
        List<String> fileList = FileManager.readFileLines(parentHash);
        List<String> filteredList = filterFileList(fileList);
        String newDirHash = HashUtil.sha256("");
        filteredList.add(".parent/\t" + parentHash);

        List<String> dirStructure = buildDirStructure(path, filteredList, newDirHash);
        Collections.sort(dirStructure);
        String newDirData = String.join("\n", dirStructure) + "\n";
        return FileManager.writeFile(HashUtil.sha256(newDirData), newDirData);
    }

    private List<String> filterFileList(List<String> fileList) {
        List<String> filteredList = new ArrayList<>();
        for (String file : fileList) {
            if (!file.contains(".parent/") && !file.isEmpty()) {
                filteredList.add(file);
            }
        }
        return filteredList;
    }

    private List<String> buildDirStructure(String path, List<String> currentList, String newDirHash) {
        currentList.add(path + "/\t" + newDirHash);
        return currentList;
    }
}
