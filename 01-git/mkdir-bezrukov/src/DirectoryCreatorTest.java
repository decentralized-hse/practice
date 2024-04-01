import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;
import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;
import java.security.NoSuchAlgorithmException;


class DirectoryCreatorTest {

    private Path tempDir;
    private Path parentHashFile;
    private final String parentHashContent = "testContent";

    @BeforeEach
    void setUp() throws IOException {
        tempDir = Files.createTempDirectory("testDir");

        parentHashFile = Files.createTempFile(tempDir, "parentHash", ".txt");
        Files.writeString(parentHashFile, parentHashContent);
    }

    @Test
    void makeDirCreatesNewDirectoryWithCorrectStructure() throws IOException, NoSuchAlgorithmException {
        DirectoryCreator creator = new DirectoryCreator();
        String newDirName = "newDir";
        String newDirPath = tempDir.resolve(newDirName).toString();

        String resultHash = creator.makeDir(newDirPath, parentHashFile.toString());
    }

}
