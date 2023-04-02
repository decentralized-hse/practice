using NSec.Cryptography;

class Program
{
    static void Main(string[] args)
    {
        if (args.Length != 1)
        {
            Console.WriteLine("Expected filename as first argument");
            return;
        }

        string fileName = args[0];

        byte[] data = File.ReadAllBytes(fileName + ".root");

        var algorithm = SignatureAlgorithm.Ed25519;
        KeyCreationParameters parameters;
        parameters.ExportPolicy = KeyExportPolicies.AllowPlaintextArchiving;
        using var keyPair = Key.Create(algorithm, parameters);
        var signature = algorithm.Sign(keyPair, data);

        File.WriteAllBytes(fileName + ".pub", keyPair.Export(KeyBlobFormat.RawPublicKey));
        File.WriteAllBytes(fileName + ".sec", keyPair.Export(KeyBlobFormat.RawPrivateKey));
        File.WriteAllBytes(fileName + ".sign", signature);
    }
}