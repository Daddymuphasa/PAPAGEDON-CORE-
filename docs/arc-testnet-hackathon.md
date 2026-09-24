# PAPAGEDON on Arc Testnet

This is the fastest hackathon path: keep PAPAGEDON's real-time engine native,
then anchor demo sessions and PGX preset metadata on Arc testnet with a small
EVM registry contract.

## Why This Works

PAPAGEDON is a real-time audiovisual runtime. The expensive rendering, audio
analysis, and shader execution should stay off-chain. Arc testnet is used for
the verifiable layer:

- prove a PAPAGEDON experience existed at a given time
- identify the creator wallet
- point to demo metadata, video, preset details, or an IPFS/Arweave asset
- produce a real Arc testnet transaction for judges

## Arc Testnet

Use these network values:

```text
Network: Arc Testnet
RPC: https://rpc.testnet.arc.io
Chain ID: 5042002
Chain ID hex: 0x4cef52
Currency: USDC
Explorer: https://explorer.testnet.arc.io
Faucet: https://faucet.circle.com
```

Arc is EVM-compatible, so standard Solidity, Remix, Foundry, Hardhat, viem, and
wagmi flows apply.

## Fastest Deployment Flow

1. Add Arc testnet to MetaMask or Rabby with the values above.
2. Get testnet USDC from the Circle faucet.
3. Open Remix: https://remix.ethereum.org
4. Create `PapagedonExperienceRegistry.sol`.
5. Paste the contract from:

```text
web3/arc-testnet/contracts/PapagedonExperienceRegistry.sol
```

6. Compile with Solidity `0.8.24` or newer.
7. In Remix deploy panel, choose `Injected Provider - MetaMask`.
8. Confirm Remix is connected to chain ID `5042002`.
9. Deploy `PapagedonExperienceRegistry`.
10. Save the deployed contract address and Arc explorer transaction link.

## Register the Demo Session

Prepare metadata from:

```text
web3/arc-testnet/example-metadata/papagedon-demo-session.json
```

Upload it somewhere stable for the submission, such as IPFS, Arweave, GitHub raw,
or a public demo endpoint.

Hash the exact metadata JSON:

```powershell
Get-FileHash web3\arc-testnet\example-metadata\papagedon-demo-session.json -Algorithm SHA256
```

Convert the resulting hash to a Solidity `bytes32` by prefixing it with `0x`.

Call `registerExperience` on the deployed contract:

```text
metadataHash: 0x<sha256>
metadataUri:  <public metadata URL>
title:        PAPAGEDON Arc Testnet Demo Session
```

The emitted `ExperienceRegistered` event is the proof for the hackathon judges.

## Submission Checklist

- GitHub repo link
- short demo video showing PAPAGEDON running
- Arc testnet contract address
- Arc explorer deployment transaction
- Arc explorer `ExperienceRegistered` transaction
- one-line pitch:

```text
PAPAGEDON turns music into adaptive real-time audiovisual worlds and anchors
experience provenance on Arc testnet with USDC-native infrastructure.
```

## Optional Next Step

After the contract is deployed, add the contract address to this file and to the
metadata JSON so judges can verify the whole loop from repository to Arc testnet.
