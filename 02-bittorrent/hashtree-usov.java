import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Paths;
import java.nio.file.StandardOpenOption;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

import static java.lang.System.exit;

public class Main {

    private static final int ChunkSize = 1024;
    private static MessageDigest digest = null;

    public static void main(String[] args) {
        if (args.length != 1) {
            System.out.println("Usage: ./program filename");
            return;
        }

        try {
            digest = MessageDigest.getInstance("SHA-256");
        } catch (final NoSuchAlgorithmException ex) {
            System.out.println(ex);
            exit(1);
        }

        final String filename = args[0];
        System.out.printf("reading %s...\n", filename);
        final byte[] content = readFile(filename);
//        System.out.println(new String(content, StandardCharsets.UTF_8));

        final List<DataChunk> chunks = getChunks(content);
//        chunks.forEach(ch -> System.out.println(byteArrayToHex(ch.getHash())));
        System.out.println("building hashtree..");
        final List<byte[]> tree = buildTree(chunks);
//        tree.forEach(n -> System.out.println(byteArrayToHex(n)));

        final String outputFile = filename + ".hashtree";
        System.out.printf("saving hashtree to %s...\n", outputFile);
        writeToFile(outputFile, tree);

        System.out.println("all done!");
    }

    public static List<byte[]> buildTree(final List<DataChunk> chunks) {
        int size = chunks.size() * 2 - 1;
        List<byte[]> ans = new ArrayList<>(size);

        for (int i = 0; i < size; ++i) {
            if (i % 2 == 0) {
                ans.add(chunks.get(i/2).getHash());
            } else {
                ans.add(new byte[32]);
            }
        }

        for (int pw = 2; pw < size; pw <<= 1) {
            int start = pw - 1;
            for (int i = start; i + (pw >> 1) < size; i += pw << 1) {
                ans.set(i, hashOfTwo(ans.get(i - (pw >> 1)), ans.get(i + (pw >> 1))));
            }
        }

        return ans;
    }

    public static byte[] hashOfTwo(final byte[] left, final byte[] right) {
        final String res = String.format("%s%s", byteArrayToHex(left), byteArrayToHex(right));
        return digest.digest(res.getBytes());
    }

    public static String byteArrayToHex(byte[] a) {
        StringBuilder sb = new StringBuilder(a.length * 2);
        for (byte b: a) {
            sb.append(String.format("%02x", b));
        }
        sb.append("\n");
        return sb.toString();
    }

    private static List<DataChunk> getChunks(byte[] data) {
        if (data.length % ChunkSize != 0) {
            throw new RuntimeException("Wrong file size, must be multiple of 1KB");
        }
        int chunks = data.length / ChunkSize;
        final List<DataChunk> ans = new ArrayList<>(chunks);
        for (int i = 0; i < chunks; ++i) {
            ans.add(
                    new DataChunk(
                            Arrays.copyOfRange(data, i * ChunkSize, i * ChunkSize + ChunkSize)
                    )
            );
        }
        return ans;
    }

    private static class DataChunk {
        final byte[] content;

        public DataChunk(final byte[] content) {
            this.content = content;
        }

        public byte[] getHash() {
            return digest.digest(
                    this.content
            );
        }
    }

    private static void writeToFile(final String outputFile, final List<byte[]> tree) {
        try {
            Files.write(Paths.get(outputFile), new byte[0], StandardOpenOption.CREATE, StandardOpenOption.TRUNCATE_EXISTING);
            for (final byte[] arr : tree) {
                Files.write(Paths.get(outputFile), byteArrayToHex(arr).getBytes(), StandardOpenOption.APPEND);
            }
        } catch (final Exception ex) {
            System.out.println(ex);
            exit(1);
        }
    }

    private static byte[] readFile(final String filename) {
        try {
            return Files.readAllBytes(
                    Paths.get(filename)
            );
        } catch (IOException ex) {
            System.out.println("Exception occurred: " + ex);
            exit(1);
        }
        return null;
    }
}
