
import java.math.BigInteger;
import java.nio.charset.StandardCharsets;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;
import java.util.Scanner;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileWriter;
import java.io.IOException;
import java.util.Arrays;

class Main {
	public static byte[] getSHA(String input) throws NoSuchAlgorithmException
    {
        MessageDigest md = MessageDigest.getInstance("SHA-256");
        return md.digest(input.getBytes(StandardCharsets.UTF_8));
    }
     
    public static String toHexString(byte[] hash)
    {
        BigInteger number = new BigInteger(1, hash);
        StringBuilder hexString = new StringBuilder(number.toString(16));
        while (hexString.length() < 64)
        {
            hexString.insert(0, '0');
        }
        return hexString.toString();
    }

	public static String sha(String s) throws NoSuchAlgorithmException
    {
        return toHexString(getSHA(s));
    }

    public static String save_file(String content) throws NoSuchAlgorithmException, IOException
    {
        String hash = sha(content);
        FileWriter myWriter = new FileWriter(hash);
        myWriter.write(content);
        myWriter.close();
        return hash;
    }

    public static String read_file(String filename)
    {
        try {
            Scanner sc = new Scanner(new File(filename));
            String content = sc.useDelimiter("\\Z").next();
            sc.close();
            return content;
        } catch (FileNotFoundException ex) {
            throw new RuntimeException(
              "File " + filename +  " doesn't exist.", ex);
        }
    }

    public static int get_file_index(String hash, String filename, boolean isDirectory){
        if (isDirectory){
            filename += "/";
        } else {
            filename += ":";
        }
        String content = read_file(hash);
        String[] lines = content.split("\n");
        for (int i = 0; i < lines.length; i++) {
            String[] parts = lines[i].split("\t");
            if (parts[0].equals(filename)) {
                return i;
            }
        }
        return -1;
    }

    public static String get_hash_by_index(String hash, int i){
        String content = read_file(hash);
        String[] lines = content.split("\n");
        String[] parts = lines[i].split("\t");
        return parts[1];
    }

    public static String update(String hash, int index, String filename, String new_file_hash, String parent_hash) throws NoSuchAlgorithmException, IOException{
        String content = read_file(hash);
        content += "\n";
        if (index == -1){ //this means file isn't written in directory yet
            content += filename += ":\t" + new_file_hash + "\n";
        }
        if (!parent_hash.equals("")){
            int parent_index = get_file_index(hash, ".parent", true);
            if (parent_index == -1){
                content += ".parent/\t" + parent_hash + "\n";
            } 
        }
        String[] lines = content.split("\n");
        if (index != -1){ //this means file was already written in directory
            String[] parts = lines[index].split("\t");
            lines[index] = parts[0] + "\t" + new_file_hash;
        }
        if (!parent_hash.equals("")){
            int parent_index = get_file_index(hash, ".parent", true);
            if (parent_index != -1){
                lines[parent_index] = ".parent/\t" + parent_hash;
            } 
        }
        Arrays.sort(lines);
        String new_content = "";
        for (String line : lines) {
            if (line.length() != 0){
                new_content += line + "\n";
            }
        }
        return save_file(new_content);
    }

     public static void main(String args[]) throws NoSuchAlgorithmException, IOException
    {
        String path = args[0];
        String root_hash = args[1];
        String separator = File.separator;
        //lexical check for correct path name
        if (path.contains(separator + separator)){
            System.out.println("Error: incorrect pathname");
            System.exit(1);
        }
        if (path.startsWith(separator)){
            path = path.substring(separator.length(), path.length());
        }
        if (path.endsWith(separator)){
            path = path.substring(0, path.length()-separator.length());
        }
        Scanner scan = new Scanner(System.in);  
        String input_content = scan.useDelimiter("\\Z").next();
        scan.close();

        String[] path_parts = path.split(separator);
        int[] indexes = new int[path_parts.length];
        String[] hashes  = new String[path_parts.length];
        String current_hash = root_hash;
        int index;
        String filename;
        for (int i = 0; i < path_parts.length - 1; i++) {
            filename = path_parts[i];
            index = get_file_index(current_hash, filename, true);
            if (index == -1){
                throw new RuntimeException(
                    "File " + filename +  " doesn't exist in directory " + current_hash);
            }
            indexes[i] = index;
            hashes[i] = get_hash_by_index(current_hash, index);
            current_hash = hashes[i];
        }
        filename = path_parts[path_parts.length-1];
        indexes[path_parts.length-1] = get_file_index(current_hash, filename, false);
        String hash_to_write = save_file(input_content);
        for (int i = (path_parts.length-1); i > 0; i-- ){
            hash_to_write = update(hashes[i-1], indexes[i], path_parts[i], hash_to_write, "");
        }
        hash_to_write = update(root_hash, indexes[0], path_parts[0], hash_to_write, root_hash);
        System.out.println(hash_to_write);
    }
}
