const web3 = require("@solana/web3.js");

(async () => {
    const conn = new web3.Connection(web3.clusterApiUrl("devnet"), "confirmed");

    const wallet = web3.Keypair.generate();
    await conn.confirmTransaction(await conn.requestAirdrop(wallet.publicKey, web3.LAMPORTS_PER_SOL * 10));

    console.log(`executing transaction with wallet: ${wallet.publicKey}`);

    const transaction = new web3.Transaction().add(new web3.TransactionInstruction({
        programId: new web3.PublicKey("24unNqRZ9d7LiczbscZvF6zx9J384CruxmivDzGZDfbC"),
        data: Buffer.of(10, 11, 12, 13, 14),
        keys: [],
    }));

    const transaction_id = await web3.sendAndConfirmTransaction(conn, transaction, [wallet], { commitment: "confirmed" });
    console.log(`transaction id: ${transaction_id}`);
})()