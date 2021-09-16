# cd "/lib/x86_64-linux-gnu"
# used for finding dll symbols

for entry in "/lib/x86_64-linux-gnu"/*
do
    echo "################################ $entry ###############################################"
    nm -D $entry
done