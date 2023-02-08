import com.goterl.lazysodium.*;
import java.io.File;
import java.io.IOException;
import java.nio.file.Files;

public class CheckNasonkov {
    public static byte[] getBytes(String filename, String file_ext) {
        // Absolute path to files needed (assuming that all files are located in the same directory)
        File file_obj = new File("C:\\Децентрализованные системы\\HW1\\build\\libs\\" + filename + "." + file_ext);
        if (!file_obj.exists() || file_obj.isDirectory()) {
            System.out.println(file_obj.exists());
            System.out.println("Invalid ." + file_ext + " file provided!");
            System.exit(1);
        }

        try {
            byte[] result = Files.readAllBytes(file_obj.toPath());
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
        SodiumJava sodium = new SodiumJava("C:\\Users\\DNS\\libsodium\\x64\\Release\\v143\\dynamic\\libsodium.dll");
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