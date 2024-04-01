public class ArgumentParser {
    private final String path;
    private final String hash;
    private final boolean valid;

    public ArgumentParser(String[] args) {
        valid = args.length == 2;
        path = valid ? args[0] : "";
        hash = valid ? args[1] : "";
    }

    public boolean isValid() {
        return valid;
    }

    public String getPath() {
        return path;
    }

    public String getHash() {
        return hash;
    }
}
