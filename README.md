This program downloads files from [Illumina BaseSpace][1]. To use this program,
please first acquire access\_token following [this link][2] if you have not
already done so. You should save this token to a file, preferably
`~/.ibssecret`. After that, you can:

1. List all your files:

		ibsget -l

2. Download a file:

		ibsget fileID

If you have difficulties in compiling this program on 64-bit Linux, you can use
precompiled binary from the `bin` directory.

[1]: https://basespace.illumina.com
[2]: https://support.basespace.illumina.com/knowledgebase/articles/403618-python-run-downloader
