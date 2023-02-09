using System;
using System.Collections.Generic;
using System.IO;

class Program {
  static List<string> FindPeaks(List<string> tree) {
    List<string> peaks = new List<string>(new string[32]);
    for (int i = 0; i < 32; i++) {
      peaks[i] = new string('0', 64);
    }

    ulong start = 0;
    for (int i = 31; i >= 0; i--) {
      ulong cur = (2UL << i) - 1;
      if (start + cur <= (ulong) tree.Count) {
	// Console.WriteLine("Processing i = " + i + " currentBlock = " + cur, " blockStart = " + start);
        peaks[i] = tree[(int) (start + cur / 2UL)];
        start = start + cur + 1;
      }
    }

    return peaks;
  }

  static void Main(string[] args) {
    if (args.Length < 1) {
      Console.WriteLine("Provide a file as an argument. `.hashtree` file should be presented in the same directory");
      return;
    }

    string inputFile = args[0] + ".hashtree";
    Console.WriteLine("Reading hashtree from " + inputFile);
    List<string> tree = new List<string>();
    using(StreamReader reader = new StreamReader(inputFile)) {
      while (!reader.EndOfStream) {
        tree.Add(reader.ReadLine());
      }
    }

    List<string> peaks = FindPeaks(tree);


    string resultFile = args[0] + ".peaks";
    Console.WriteLine("Writing peaks to " + resultFile);

    using(StreamWriter writer = new StreamWriter(resultFile)) {
      foreach(string peak in peaks) {
        writer.WriteLine(peak);
      }
    }
  }
}
