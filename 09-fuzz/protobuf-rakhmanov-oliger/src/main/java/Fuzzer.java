import com.code_intelligence.jazzer.api.FuzzedDataProvider;
import java.io.FileOutputStream;
import java.io.FileWriter;
import java.io.IOException;
import java.io.PrintWriter;
import java.lang.Exception;
import java.nio.BufferUnderflowException;
import java.nio.file.Files;
import java.nio.file.Paths;
import java.util.Arrays;

public class Fuzzer {
  private static void writeBytesToFile(byte[] data, String filePath) {
    try (FileOutputStream fos = new FileOutputStream(filePath)) {
      fos.write(data);
      fos.close();
    } catch (IOException e) {
      e.printStackTrace();
      // exit(1);
    }
  }

  private static boolean checkDiff(final byte[] bytes) throws Exception {
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

  public static void fuzzerTestOneInput(FuzzedDataProvider fuzzedDataProvider)
      throws Exception {
    final byte[] bytes = fuzzedDataProvider.consumeRemainingAsBytes();
    writeBytesToFile(bytes, "input.bin");
    // PrintWriter writer = new PrintWriter(new FileWriter("result.txt", true));
    // boolean check = true;

    try {
      // check = true;
      Main.main(new String[] {"input.bin"});
      Main.main(new String[] {"input.protobuf"});

      if (!checkDiff(bytes)) {

        System.err.printf("\n---GOOD---\n");
        System.err.printf(Arrays.toString(bytes));
        byte[] array = Files.readAllBytes(Paths.get("input.bin"));
        System.err.printf("\n---BAD----\n");
        System.err.printf(Arrays.toString(array));
        System.err.printf("\n----------\n");

        throw new RuntimeException("Fuzzing found a mismatch!");
      } else {
      }
    } catch (IllegalArgumentException e) {
      // do nothing
      // check = false;
    } catch (final Exception e) {
      if (!(e instanceof IllegalArgumentException)) {

        // System.err.printf("ERROR THROWN HERE!\n");
        // throw e;

        PrintWriter writer =
            new PrintWriter(new FileWriter("result.txt", true));
        e.printStackTrace(writer);
        writer.close();

        // exit(1);
      }
    }
  }
}
