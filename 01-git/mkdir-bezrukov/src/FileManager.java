import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.List;

public class FileManager {
    public static List<String> readFileLines(String path) throws IOException {
        return Files.readAllLines(Path.of(path));
    }

    public static String writeFile(String fileName, String data) throws IOException {
        Files.write(Path.of(fileName), data.getBytes());
        return fileName;
    }
}
