async function replicateChessApp(targetPath) {
    const sourcePath = '/chess/';
    const manifest = await fetch(sourcePath + 'manifest.json').then(r => r.json());
    const files = [
        'index.html',
        'chess.js',
        'style.css',
        'manifest.json',
        'replicator.js',
        'games/template/metadata.json',
        'games/template/empty_moves.json'
    ];
    for (const file of files) {
        const content = await fetch(sourcePath + file).then(r => r.text());
        await fetch(targetPath + file, {
            method: 'PUT',
            body: content
        });
        console.log(`Replicated: ${file}`);
    }
    console.log(`Chess app replicated to ${targetPath}`);
}

if (import.meta.url === `file://${process.argv[1]}`) {
    const target = process.argv[2] || '/chess-copy/';
    replicateChessApp(target);
}
