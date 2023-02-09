import java.io.File;
import java.io.FileNotFoundException;
import java.util.*;
import java.io.FileWriter;
import java.io.IOException;

import static java.lang.System.exit;
public class Main {
    private static final int NUM_LEVELS = 32;
    private static final int HASH_SIZE = 64;
    public static void main(String[] args) {
        if (args.length != 1) {
            System.out.println("Invalid number of arguments");
            return;
        }
        String filename = args[0];
        String[] hashes = readFile(filename + ".hashtree");
        writeFile(filename, getPeaks(hashes));
    }

    private static String[] getPeaks(String[] hashes) {
        String[] peaks = new String[NUM_LEVELS];
        String zeros = new String(new char[HASH_SIZE]).replace('\0', '0');
        for (int i = 0; i < peaks.length; i++) {
            peaks[i] = zeros;
        }
        int firstOccur = 0, secondOccur = 0;
        boolean seqStart = false;
        int subtree, index, numlayer;
        for (int i = 0; i < hashes.length; i++) {
            if (hashes[i].equals(zeros)) {
                if (seqStart) {
                    seqStart = false;
                    subtree = secondOccur - firstOccur + 1;
                    index = (secondOccur + firstOccur) / 2;
                    numlayer = (32 - Integer.numberOfLeadingZeros(subtree)) - 1;
                    peaks[numlayer] = hashes[index];
                }
            } else {
                secondOccur = i;
                if (!seqStart) {
                    seqStart = true;
                    firstOccur = i;
                }
            }
        }
        if (seqStart) {
            seqStart = false;
            subtree = secondOccur - firstOccur + 1;
            index = (secondOccur + firstOccur) / 2;
            numlayer = (32 - Integer.numberOfLeadingZeros(subtree)) - 1;
            peaks[numlayer] = hashes[index];
        }
        return peaks;
    }
    private static void writeFile(String filename, String[] peaks) {
        try {
            FileWriter myWriter = new FileWriter(filename + ".peaks");
            for (String el : peaks) {
                myWriter.write(el);
                myWriter.write('\n');
            }
            myWriter.close();
            System.out.println("Successfully wrote to the file.");
        } catch (IOException e) {
            System.out.println("An error occurred.");
            e.printStackTrace();
            exit(1);
        }
    }
    private static String[] readFile(String filename) {
        try {
            ArrayList<String> data = new ArrayList<String>();
			Scanner scanner = new Scanner(new File(filename));

			while (scanner.hasNextLine()) {
                data.add(scanner.nextLine());
			}

			scanner.close();
            String[] hashes = new String[data.size()];
            data.toArray(hashes);
            return hashes;
		} catch (FileNotFoundException e) {
			e.printStackTrace();
            exit(1);
		}
        return null;
    }
}