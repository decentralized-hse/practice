import com.goterl.lazysodium.*;
import java.io.File;
import java.io.IOException;
import java.nio.file.Files;
import java.util.HexFormat;

public class CheckNasonkov {
    public static byte[] getBytes(String filename, String file_ext) {
        // Absolute path to files needed (assuming that all files are located in the same directory)
        File file_obj = new File(filename + "." + file_ext);
        if (!file_obj.exists() || file_obj.isDirectory()) {
            System.out.println(file_obj.exists());
            System.out.println("Invalid ." + file_ext + " file provided!");
            System.exit(1);
        }

        try {
            byte[] result;
            if (file_ext != "root") {
                HexFormat hex = HexFormat.of();
                String hex_bytes_str = Files.readString(file_obj.toPath());
                result = hex.parseHex(hex_bytes_str.substring(0, hex_bytes_str.length() - 1));
            } else {
                result = Files.readAllBytes(file_obj.toPath());
            }
            return result;
        } catch(IOException e) {
            System.out.println("IOException while reading bytes from file occured!");
            e.printStackTrace();
            System.exit(1);
        }
        return new byte[1];
    }

    public static void main(String[] args) {
        // Absolute path to libsodium.dll needed
        SodiumJava sodium = new SodiumJava("/usr/lib/x86_64-linux-gnu/libsodium.so.23.3.0");
        LazySodiumJava lazySodium = new LazySodiumJava(sodium);

        if (args.length == 0) {
            System.out.println("Expected filename as command line argument!");
            return;
        }

        String filename = args[0];
        byte[] pub_key_bytes = getBytes(filename, "pub");
        byte[] msg_bytes = getBytes(filename, "root");
        byte[] sign_bytes = getBytes(filename, "sign");

        boolean checking_result = lazySodium.cryptoSignVerifyDetached(sign_bytes,
                                                                      msg_bytes,
                                                                      msg_bytes.length,
                                                                      pub_key_bytes);
        if (checking_result) {
            System.out.println("Signature is correct");
            System.exit(0);
        } else {
            System.out.println("Signature is invalid!");
            System.exit(1);
        }
    }
}
