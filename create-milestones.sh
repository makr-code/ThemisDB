#!/bin/bash

# Script to create milestones using GitHub CLI

# Define milestones and descriptions
declare -A milestones
milestones=(
  ["v1.5.0"]="Description for version 1.5.0"
  ["v1.6.0"]="Description for version 1.6.0"
  ["v1.7.0"]="Description for version 1.7.0"
  ["v1.8.0"]="Description for version 1.8.0"
  ["v1.9.0"]="Description for version 1.9.0"
  ["v2.0.0"]="Description for version 2.0.0"
  ["v2.1.0"]="Description for version 2.1.0"
  ["v2.2.0"]="Description for version 2.2.0"
  ["v2.3.0"]="Description for version 2.3.0"
)

# Create milestones
for version in "${!milestones[@]}"; do
  gh milestone create "$version" --description "${milestones[$version]}"
done
