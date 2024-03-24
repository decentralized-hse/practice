import java.io.File;
import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Paths;

public class GitGet {
    private static final String DIR_SEPARATOR = "/\t";
    private static final String FILE_SEPARATOR = ":\t";

    public static void main(String[] args) {
        String[] path = args[0].split("/");
        String rootHash = args[1];

        String prefix = ".";
        for (int i = 0; i < path.length; i++) {
            String token = path[i];
            if (!new File(rootHash).exists()) {
                System.out.println("Non-existing root hash: " + rootHash);
                System.exit(1);
            }
            try {
                String data = new String(Files.readAllBytes(Paths.get(rootHash)));
                if (i == path.length - 1) {
                    rootHash = findNewRootHash(data, token, FILE_SEPARATOR);
                    if (rootHash == null) {
                        System.out.println("Failed to find " + token);
                        System.exit(1);
                    }
                    System.out.print(new String(Files.readAllBytes(Paths.get(rootHash))));
                    break;
                }
                rootHash = findNewRootHash(data, token, DIR_SEPARATOR);
                if (rootHash == null) {
                    System.out.println("Failed to find " + token);
                    System.exit(1);
                }
            } catch (IOException e) {
                e.printStackTrace();
            }
        }
    }

    private static String findNewRootHash(String rootHashData, String blobName, String separator) {
        String[] lines = rootHashData.split("\n");
        for (String line : lines) {
            String[] tokens = line.split(separator);
            if (tokens.length != 2) {
                continue;
            }
            String name = tokens[0].trim();
            String hash = tokens[1].trim();
            if (name.equals(blobName)) {
                return hash;
            }
        }
        return null;
    }
}
