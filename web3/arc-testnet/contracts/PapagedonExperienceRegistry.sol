// SPDX-License-Identifier: MIT
pragma solidity ^0.8.24;

/// @title PAPAGEDON Experience Registry
/// @notice Anchors PAPAGEDON presets or rendered sessions on Arc testnet.
/// @dev Store the asset JSON off-chain, then submit its content hash here.
contract PapagedonExperienceRegistry {
    struct Experience {
        address creator;
        bytes32 metadataHash;
        string metadataUri;
        string title;
        uint64 createdAt;
    }

    Experience[] private experiences;

    event ExperienceRegistered(
        uint256 indexed experienceId,
        address indexed creator,
        bytes32 indexed metadataHash,
        string metadataUri,
        string title
    );

    function registerExperience(
        bytes32 metadataHash,
        string calldata metadataUri,
        string calldata title
    ) external returns (uint256 experienceId) {
        require(metadataHash != bytes32(0), "metadata hash required");
        require(bytes(metadataUri).length != 0, "metadata uri required");
        require(bytes(title).length != 0, "title required");

        experienceId = experiences.length;
        experiences.push(
            Experience({
                creator: msg.sender,
                metadataHash: metadataHash,
                metadataUri: metadataUri,
                title: title,
                createdAt: uint64(block.timestamp)
            })
        );

        emit ExperienceRegistered(
            experienceId,
            msg.sender,
            metadataHash,
            metadataUri,
            title
        );
    }

    function experienceCount() external view returns (uint256) {
        return experiences.length;
    }

    function getExperience(uint256 experienceId)
        external
        view
        returns (Experience memory)
    {
        require(experienceId < experiences.length, "unknown experience");
        return experiences[experienceId];
    }
}
