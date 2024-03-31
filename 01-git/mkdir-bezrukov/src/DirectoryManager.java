public class DirectoryManager {
    public static void main(String[] args) {
        ArgumentParser parser = new ArgumentParser(args);
        if (!parser.isValid()) {
            System.out.println("invalid input");
            return;
        }

        DirectoryCreator directoryCreator = new DirectoryCreator();
        try {
            String resultHash = directoryCreator.makeDir(parser.getPath(), parser.getHash());
            System.out.println("Hash: " + resultHash);
        } catch (Exception e) {
            e.printStackTrace();
            System.exit(1);
        }
    }
}
