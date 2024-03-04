package rm;

import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;
import java.util.Arrays;

import java.io.IOException;

import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;


import com.beust.jcommander.JCommander;
import com.beust.jcommander.Parameter;

public class MyRm {

    @Parameter(names = {"--path"}, description = "Path")
    private String path;

    @Parameter(names = {"--hash"}, description = "Hash")
    private String hash;

    public static void main(String[] argv) throws IOException, NoSuchAlgorithmException {
        MyRm main = new MyRm();
        JCommander.newBuilder()
                .addObject(main)
                .build()
                .parse(argv);

        Path filePath = Paths.get(main.path);

        System.out.println("Let's start");
        System.out.println(main.hash);

        List<String> lines = Arrays.asList(filePath.toString().split("/"));

        String ans = iterateAndRemove(lines.iterator(), main.hash, true);
        System.out.println(ans);
    }

    public static String iterateAndRemove(Iterator<String> pathIter, String hash, boolean isRoot) throws IOException, NoSuchAlgorithmException {
        if (!pathIter.hasNext()) {
            return null;
        }

        String nextDir = pathIter.next();
        List<String> lines = Files.readAllLines(Paths.get(hash));

        String nextHash = lines.stream()
                .filter(line -> line.startsWith(nextDir))
                .map(line -> line.split("\t")[1])
                .findFirst()
                .orElse(null);
        System.out.println(nextHash);
        System.out.println(nextDir);

        String newHash = iterateAndRemove(pathIter, nextHash, false);

        List<String> ans = new ArrayList<>();

        for (String line : lines) {
            if (line.startsWith(nextDir) && newHash != null) {
                ans.add(nextDir + "/\t" + newHash);
            } else if (!line.startsWith(nextDir) && !line.startsWith(".parent")) {
                ans.add(line);
            }
        }

        if (isRoot) {
            ans.add(".parent/\t" + hash);
        }

        ans.sort(null);
        ans.add("");

        String binding = String.join("\n", ans);
        byte[] bytes = binding.getBytes();
        String totalHash = digest(bytes);

        Files.write(Paths.get(totalHash), bytes);

        return totalHash;
    }

    public static String digest(byte[] bytes) throws NoSuchAlgorithmException {
        MessageDigest md = MessageDigest.getInstance("SHA-256");
        byte[] hashBytes = md.digest(bytes);

        // Преобразование хеш-значения в шестнадцатеричную строку
        StringBuilder hexString = new StringBuilder();
        for (byte b : hashBytes) {
            String hex = Integer.toHexString(0xff & b);
            if (hex.length() == 1) hexString.append('0');
            hexString.append(hex);
        }

        return hexString.toString();
    }
}