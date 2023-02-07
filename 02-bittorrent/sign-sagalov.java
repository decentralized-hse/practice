// To simplify the structure...
// package ru.hse.decentralized.systems.sagalov;

import java.io.*;
import java.nio.charset.StandardCharsets;
import java.security.*;
import java.util.Base64;

public class Main {
    public static final String ENCRYPTION_ALGORITHM = "Ed25519";

    public static void main(String[] args) {
        if (args.length == 0) {
            System.out.println("No command line arguments provided, please provide a file to sign.");
            return;
        }

        try {
            byte[] msg = readFileContentsAsUtf8(args[0]);

            // Generate the public/private key pair using Ed25519.
            KeyPair pubPrivPair = getEd25519KeyPair();
            PrivateKey privateKey = pubPrivPair.getPrivate();

            // Sign the file contents with an Ed25519 private key.
            byte[] signedBytes = signDataWithPrivateKey(privateKey, msg);

            byte[] publicKeyBytes = pubPrivPair.getPublic().getEncoded();
            byte[] privateKeyBytes = privateKey.getEncoded();

            // Write data to files in Base64.
            Base64.Encoder encoder = Base64.getEncoder();
            writeEncodedKeysToFiles(encoder, publicKeyBytes, "key.pub");
            writeEncodedKeysToFiles(encoder, privateKeyBytes, "key.sec");
            writeEncodedKeysToFiles(encoder, signedBytes, "data.sign");
        } catch (Exception e) {
            System.out.println(e.getMessage());
        }
    }

    private static byte[] readFileContentsAsUtf8(String filePath) throws IOException {
        try (BufferedReader bufferedReader = new BufferedReader(new FileReader(filePath))) {
            StringBuilder sb = new StringBuilder();
            while (bufferedReader.ready()) {
                sb.append(bufferedReader.readLine());
            }
            String encryptedMessage = sb.toString();
            return encryptedMessage.getBytes(StandardCharsets.UTF_8);
        }
    }

    private static KeyPair getEd25519KeyPair() throws NoSuchAlgorithmException {
        KeyPairGenerator ed25519Generator = KeyPairGenerator.getInstance(ENCRYPTION_ALGORITHM);
        return ed25519Generator.genKeyPair();
    }

    private static byte[] signDataWithPrivateKey(PrivateKey privateKey, byte[] data)
            throws NoSuchAlgorithmException, InvalidKeyException, SignatureException {
        Signature ed25519Signature = Signature.getInstance(ENCRYPTION_ALGORITHM);
        ed25519Signature.initSign(privateKey);
        ed25519Signature.update(data);

        return ed25519Signature.sign();
    }

    private static void writeEncodedKeysToFiles(Base64.Encoder encoder, byte[] data, String path) throws IOException {
        try (BufferedWriter bufferedWriter = new BufferedWriter(new FileWriter(path))) {
            bufferedWriter.write(encoder.encodeToString(data));
        }
    }
}