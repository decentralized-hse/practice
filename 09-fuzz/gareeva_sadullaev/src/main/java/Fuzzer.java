import java.io.FileOutputStream;
import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Paths;
import java.util.Arrays;

import com.code_intelligence.jazzer.api.FuzzedDataProvider;

public class Fuzzer {
    private static void writeBytesToFile(byte[] data, String filePath) {
        try (FileOutputStream fos = new FileOutputStream(filePath)) {
            fos.write(data);
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    private static boolean compare(byte[] bytes) throws IOException {
        byte[] fileBytes = Files.readAllBytes(Paths.get("first.bin"));
        return Arrays.equals(fileBytes, bytes);
    }

    public static void fuzzerTestOneInput(FuzzedDataProvider fuzzedDataProvider) throws Exception {
        final byte[] bytes = fuzzedDataProvider.consumeRemainingAsBytes();
        writeBytesToFile(bytes, "first.bin");

        try {
            Main.formatJsonToBin(Paths.get("students.json"), Paths.get("students.bin"));
            Main.formatBinToJson(Paths.get("students.bin"), Paths.get("students.json"));

            if (!compare(bytes)) {
                throw new RuntimeException("Данные не совпадают");
            }
        } catch (RuntimeException error) {
            System.err.println("======= GOOD =======");
        }

    }
}