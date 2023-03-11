# FAQ and Known Errors

## Install Error ``bash: line 1: syntax error near unexpected token `<'``
If this happens, the DNS redirect (vanity URL) I use to make the install command shorter and easier to type may have broken.

**Explanation:**  The installation command line uses an application called `curl` to download the target URL. The pipe operator (`|`) redirects that to whatever follows, in this case, `sudo` (run as root) `bash` (the command interpreter) to make the bash script run as soon as it downloads. A regular HTML page will be sent instead of the bash script if the redirect breaks somehow. Bash doesn't know what to do with HTML (the `<` in the first position of the first line), so it simply refuses to do anything.

You may use the following longer and more difficult to type, command instead (one line):

```bash
curl -L https://raw.githubusercontent.com/lbussy/WsprryPi/main/scripts/install.sh | sudo bash
```
