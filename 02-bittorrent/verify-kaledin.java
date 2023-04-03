package com.company;

import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Paths;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;

import static java.lang.System.exit;

public class HW1 {

    private static final String zeroes = "0000000000000000000000000000000000000000000000000000000000000000";

    public static void main(String[] args) throws IOException, NoSuchAlgorithmException {
        if (args.length != 2) {
            System.out.println("Not enough arguments");
            exit(1);
        }

        String filename = args[0];
        long chunk_index = Long.parseLong(args[1]);
        MessageDigest digest = MessageDigest.getInstance("SHA-256");


        // Verify Peaks
        System.out.println(filename + ".peaks");
        byte[] contentPeaks = Files.readAllBytes(Paths.get(filename + ".peaks"));
        System.out.println(filename + ".root");
        byte[] contentRoot = Files.readAllBytes(Paths.get(filename + ".root"));
        byte[] hashRoot = digest.digest(contentPeaks);
        if (!new String(hashRoot, StandardCharsets.UTF_8).equals(new String(contentRoot, StandardCharsets.UTF_8))) {
            System.out.println("Peaks do not match :(");
            exit(1);
        }
        System.out.println("Peaks match :)");


        // Verify Chunk
        System.out.println(filename + '.' + chunk_index + ".chunk");
        byte[] contentChunk = Files.readAllBytes(Paths.get(filename + '.' + chunk_index + ".chunk"));
        System.out.println(filename + ".peaks");
        var peaks = Files.readAllLines(Paths.get(filename + ".peaks"));

        long counter = 0, high = -1;
        for (int i = peaks.size() - 1; i >= 0; i--) {
            if (zeroes.equals(peaks.get(i))) {
                counter += (1L << i);
                if (counter > chunk_index) {
                    high = i;
                    break;
                }
            }
        }
        long index = (1L << high) + chunk_index - counter;

        String hashChunk = new String(digest.digest(contentChunk), StandardCharsets.UTF_8);
        System.out.println(filename + '.' + chunk_index + ".proof");
        var proofLines = Files.readAllLines(Paths.get(filename + '.' + chunk_index + ".proof"));
        for (int i = 0; i < high; i++) {
            if (index % 2 == 1) {
                hashChunk = new String(
                        digest.digest((proofLines.get(i) + "\n" + hashChunk + "\n").getBytes(StandardCharsets.UTF_8)),
                        StandardCharsets.UTF_8
                );
            } else {
                hashChunk = new String(
                        digest.digest((hashChunk + "\n" + proofLines.get(i) + "\n").getBytes(StandardCharsets.UTF_8)),
                        StandardCharsets.UTF_8
                );
            }
            index /= 2;
            if (!peaks.get(i + 1).equals(hashChunk)) {
                System.out.println("Failed verifying chunks :(");
                exit(1);
            }
        }

        System.out.println("!!! Everything is OK !!!");
    }
}
