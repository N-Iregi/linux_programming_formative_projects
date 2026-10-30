#!/bin/bash
# Static analysis only. The target binaries are NEVER executed
# and ldd is NOT used. Run after compiling both programs.

mkdir -p outputs

for bin in data_sync data_sync_v2; do
	ls -l "$bin"                 > "outputs/${bin}_ls.txt" 2>&1
	sha256sum "$bin"             > "outputs/${bin}_sha256.txt" 2>&1
	file "$bin"                  > "outputs/${bin}_file.txt" 2>&1
	readelf -h "$bin"            > "outputs/${bin}_readelf_h.txt" 2>&1
	readelf -S "$bin"            > "outputs/${bin}_readelf_S.txt" 2>&1
	readelf -l "$bin"            > "outputs/${bin}_readelf_l.txt" 2>&1
	readelf -s "$bin"            > "outputs/${bin}_readelf_s.txt" 2>&1
	readelf -d "$bin"            > "outputs/${bin}_readelf_d.txt" 2>&1
	nm -n "$bin"                 > "outputs/${bin}_nm_n.txt" 2>&1
	nm -D "$bin"                 > "outputs/${bin}_nm_D.txt" 2>&1
	objdump -h "$bin"            > "outputs/${bin}_objdump_h.txt" 2>&1
	objdump -t "$bin"            > "outputs/${bin}_objdump_t.txt" 2>&1
	objdump -T "$bin"            > "outputs/${bin}_objdump_T.txt" 2>&1
	objdump -d "$bin"            > "outputs/${bin}_objdump_d.txt" 2>&1
	objdump -p "$bin"            > "outputs/${bin}_objdump_p.txt" 2>&1
	strings -a "$bin"            > "outputs/${bin}_strings.txt" 2>&1
done

diff outputs/data_sync_nm_D.txt outputs/data_sync_v2_nm_D.txt \
	> outputs/diff_imports.txt 2>&1

echo "Done. Results saved in the outputs/ folder."
