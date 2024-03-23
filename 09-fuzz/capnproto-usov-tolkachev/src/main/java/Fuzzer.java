import java.io.FileOutputStream;
import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Paths;

import com.code_intelligence.jazzer.api.FuzzedDataProvider;

public class Fuzzer {
    private static void writeBytesToFile(byte[] data, String filePath) {
        try (FileOutputStream fos = new FileOutputStream(filePath)) {
            fos.write(data);
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    private static boolean compare(final byte[] bytes) throws Exception {
    byte[] file = Files.readAllBytes(Paths.get("input.bin"));

    if (file.length != file.length) {
      return false;
    } else {
      boolean isEqual = true;
      for (int i = 0; i < file.length; i++) {
        if (file[i] != bytes[i]) {
          isEqual = false;
          break;
        }
      }

      return isEqual;
    }
  }

    public static void fuzzerTestOneInput(FuzzedDataProvider fuzzedDataProvider) throws Exception {
        final byte[] bytes = fuzzedDataProvider.consumeRemainingAsBytes();
        writeBytesToFile(bytes, "input.bin");

        try {
          Main.toCapnproto("input.bin");
          Main.toBin("input.capnproto");

          if (!compare(bytes)) {
              throw new RuntimeException("Fuzzing found a mismatch");
          }
        } catch (final IllegalArgumentException e) {
          // this is fine
        }
        


    }
}
