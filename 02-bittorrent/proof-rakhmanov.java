import java.io.File;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.nio.charset.Charset;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.ArrayList;
import java.util.Scanner;

public class Main {

    public static ArrayList<String> readFile(String inFileName) {
        ArrayList<String> result = new ArrayList<>();
        Scanner inFile = null;
        try {
            inFile = new Scanner(new File(inFileName));
        } catch (FileNotFoundException ex) {
            ex.printStackTrace();
        }
        if (inFile != null) {
            while (inFile.hasNextLine()) {
                result.add(inFile.nextLine());
            }
        }
        return result;
    }

    private static int siblingIndex(int node, int level) {
        return (node ^ (1 << level));
    }

    private static int nextNode(int node, int level) {
        return (node + siblingIndex(node, level)) >> 1;
    }

    private static int nextUpperBound(int node, int level) {
        return node + (1 << level) - 1;
    }

    public static ArrayList<String> getProof(ArrayList<String> contents, int index) {
        int node = index * 2;
        int upperBound = node;
        int level = 0;
        ArrayList<String> result = new ArrayList<>();
        while (upperBound < contents.size()) {
            level++;
            int oldNode = node;
            node = nextNode(node, level);
            upperBound = nextUpperBound(node, level);
            if (upperBound < contents.size()) {
                result.add(contents.get(siblingIndex(oldNode, level)));
            }
        }
        return result;
    }

    public static void writeFile(ArrayList<String> contents, String outFileName) {
        Path out = Paths.get(outFileName);
        try {
            Files.write(out, contents, Charset.defaultCharset());
        } catch (IOException ex) {
            ex.printStackTrace();
        }
    }
    public static void main(String[] args) {
        if (args.length != 2) {
            System.out.println("Please provide file name and chunk index.");
            return;
        }
        String inFileName = args[0];
        String inFileNameHashTree = inFileName + ".hashtree";
        int chunkIndex = Integer.parseInt(args[1]);
        System.out.println("Reading " + inFileNameHashTree + "...");
        var lines = readFile(inFileNameHashTree);
        System.out.println("Creating proof...");
        var proof = getProof(lines, chunkIndex);
        String outFileName = inFileName + "." + chunkIndex + ".proof";
        System.out.println("Writing proof to " + outFileName + "...");
        writeFile(proof, outFileName);
    }
}
