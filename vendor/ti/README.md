# Vendored TI reference documents (local only, never committed)

This directory holds local copies of the Texas Instruments C54x
reference PDFs that ground the instruction evidence in
`config/evidence.json`. The PDFs themselves are intentionally
git-ignored (see `.gitignore`); only this README is tracked. It
records exactly what was fetched, from where, and how to refresh it,
so any checkout can reproduce the reference set without web access
being required at build time.

No PDF content is quoted into sources: evidence records cite
chapter/section/printed page, and opcode tables are re-derived from
those citations, never copied.

| File           | Document                                                                 | Official URL                                    | Fetched from        | Access date |
|----------------|--------------------------------------------------------------------------|-------------------------------------------------|---------------------|-------------|
| `spru172c.pdf` | SPRU172C, TMS320C54x DSP Mnemonic Instruction Set, Vol. 2, Rev. C (2001-03) | https://www.ti.com/lit/ug/spru172c/spru172c.pdf | alexrp mirror (docs.alexrp.com/tic54x/tic54x_mnemonic.pdf), verified byte-identical against ti.com official on 2026-09-11 | 2026-09-10 |
| `spru179c.pdf` | SPRU179C, TMS320C54x DSP Algebraic Instruction Set, Vol. 3, Rev. C (2001-03) | https://www.ti.com/lit/ug/spru179/spru179.pdf   | alexrp mirror (docs.alexrp.com/tic54x/tic54x_algebraic.pdf), verified byte-identical against ti.com official on 2026-09-11 | 2026-09-10 |
| `spru131g.pdf` | SPRU131G, TMS320C54x DSP CPU and Peripherals, Vol. 1, Rev. G (2001-03)     | https://www.ti.com/lit/ug/spru131g/spru131g.pdf | ti.com (official) | 2026-09-10 |

SHA-256 of the vendored files (verify after any refresh):

- `spru172c.pdf`: `c4f89ce4a774a76de1d93a2413e882a71b82d236fc2c52af63f3159368a37e75`
- `spru179c.pdf`: `0885253bb5f11aa35c41297f96a428063edd9d167669974aea5a33eee3780415`
- `spru131g.pdf`: `d9ea0e695859d605d1f94dabeede327c0e343099102cfd7e224f6f53375a3fa2`

Mirror caveat: the 172C/179C copies came from a third-party mirror
because the direct TI fetch was not attempted first; prefer
re-fetching those two from the official URLs above when refreshing,
and update the hashes here. 2026-09-11: the 172C mirror copy was
verified byte-identical (md5 `da20fb3bec107aa81f84724acd8e3af5`,
sha256 as above) against a fresh ti.com official download; the
179C copy verified the same way on 2026-09-11 (md5
`c8d0bbe29b1eee8b109b1dea62d4f8c4`, sha256 as above).

Text extractions (`spru172c.txt`, `spru179c.txt`, `spru131g.txt`,
74,350 lines total) are pre-generated here with the same command and
are equally git-ignored — evidence work greps them directly instead
of re-extracting every turn:

    pdftotext -layout vendor/ti/spru172c.pdf vendor/ti/spru172c.txt
    pdftotext -layout vendor/ti/spru179c.pdf vendor/ti/spru179c.txt
    pdftotext -layout vendor/ti/spru131g.pdf vendor/ti/spru131g.txt
