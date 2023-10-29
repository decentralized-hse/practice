using ParseLibrary;
using SharpFuzz;

Fuzzer.OutOfProcess.Run(stream =>
{
    try
    {
        using var reader = new StreamReader(stream);
        MainParser.FromKvFile(reader);
    }
    catch (ParseException)
    {
    }
});


// try
// {
//     using (var reader = new StreamReader(Path.Combine("Testcases", "test.kv")))
//     {
//         MainParser.FromKvFile(reader);
//         // JSON.DeserializeDynamic(reader);
//     }
// }
// catch (ParseException e)
// {
//     // ignored
// }