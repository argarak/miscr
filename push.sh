#!/bin/zsh

git add .
printf "Commit message: "
read commit
git commit -m "$commit"
printf "branch? "
read repository
git push origin $repository
