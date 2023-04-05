import java.security.*;
import java.nio.file.Files;
import java.nio.file.Paths;
import java.util.Arrays;

import org.bouncycastle.jce.provider.BouncyCastleProvider;
import org.bouncycastle.util.encoders.Hex;

public class Main {
    public static final String ENCRYPTION_ALGORITHM = "Ed25519";
    public static final String BOUNCY_CASTLE_CRYPTO_PROVIDER = "BC";

    public static void main(String[] args) {
        Security.addProvider(new BouncyCastleProvider());
        if (args.length == 0) {
            System.out.println("No command line arguments provided, please provide a file to sign.");
            return;
        }

        try {
            byte[] fileContents = Files.readAllBytes(Paths.get(args[0] + ".root"));
            KeyPairGenerator keyPairGenerator = KeyPairGenerator
                    .getInstance(ENCRYPTION_ALGORITHM, BOUNCY_CASTLE_CRYPTO_PROVIDER);
            KeyPair keyPair = keyPairGenerator.generateKeyPair();
            PrivateKey privateKey = keyPair.getPrivate();
            PublicKey publicKey = keyPair.getPublic();

            Signature signature = Signature.getInstance(ENCRYPTION_ALGORITHM, BOUNCY_CASTLE_CRYPTO_PROVIDER);
            signature.initSign(privateKey);
            signature.update(fileContents);
            byte[] signedFileContents = signature.sign();

            byte[] encodedPublicKey = publicKey.getEncoded();
            byte[] rawPublicKey = Arrays.copyOfRange(encodedPublicKey, encodedPublicKey.length - 32, encodedPublicKey.length);
            byte[] encodedPrivateKey = privateKey.getEncoded();
            byte[] rawPrivateKey = Arrays.copyOfRange(encodedPrivateKey, encodedPrivateKey.length - 67, encodedPrivateKey.length - 35);

            Files.write(Paths.get(args[0] + ".pub"), Hex.encode(rawPublicKey));
            Files.write(Paths.get(args[0] + ".sec"), Hex.encode(rawPrivateKey));
            Files.write(Paths.get(args[0] + ".sign"), Hex.encode(signedFileContents));
        } catch (Exception e) {
            System.out.println(e.getMessage());
        }
    }
}