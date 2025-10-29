# Bill of Materials Workflow

This folder contains everything you need to track, extend, and cost out the headset and drone hardware. The system is intentionally simple so you can add new parts without learning a CAD program or database tool.

---

## 1. File Overview

| File | Purpose |
|------|---------|
| `bom_headset.csv` | Base parts for the headset node |
| `bom_drone.csv` | Base parts for the drone node |
| `bom_master.csv` | Auto-generated combined list (do not edit by hand) |
| `bom_examples.csv` | Sample upgrades and optional accessories |
| `bom.md` | This document |
| `../tools/bom_aggregate.py` | Python script that merges and validates CSVs |
| `../Makefile` | Provides the `make bom` shortcut |
| `printable_label_cards.pdf` | Cards for labeling bags and harnesses |

---

## 2. Editing the BoM

1. Open `bom_headset.csv` or `bom_drone.csv` in a spreadsheet editor (Excel, LibreOffice, Google Sheets).
2. Keep the header row **exactly** as provided:
   ```text
   Category,Subcategory,Item,Manufacturer,Model/Part#,Supplier,Supplier SKU,URL,Qty,Unit,Unit Price (USD),Total Price (USD),Unit Weight (g),Total Weight (g),Voltage,Current,Notes,Version/Rev,Footprint/Size,Where Installed
   ```
3. Add or edit rows as needed. Fill in quantities, units, and prices; the script will compute totals.
4. Save the file as CSV (UTF-8) and commit your changes.

---

## 3. Generating the Master BoM

Run the following command from the repository root:

```bash
make bom
```

This invokes `python3 tools/bom_aggregate.py` which:

* Validates that required columns exist and that numeric fields contain valid numbers.
* Combines the headset and drone CSVs (plus any others you add) into `hardware/bom_master.csv`.
* Creates a human-readable summary at `hardware/bom_master.md` with subtotals by category and by build section.
* Prints overall cost and weight totals to the console.

If validation fails the script explains the problem and exits with a non-zero status so CI pipelines can catch mistakes.

---

## 4. Interpreting the Results

The generated `bom_master.md` contains:

* **Grand totals** – total cost and weight for the entire system.
* **Section breakdown** – separate sums for headset and drone (look for the `Build Section` column).
* **Category subtotals** – e.g., Electronics, Fasteners, Printed Parts.
* **Detailed table** – every item, quantity, price, and where it installs.

Use the totals to estimate your build budget and payload weight. Update the CSVs if you swap in heavier or lighter components.

---

## 5. Adding New Build Sections

If you want to track extra categories (e.g., "Ground Station"), create another CSV with the same header and run `make bom`. The script automatically picks up any CSV in `hardware/` that matches `bom_*.csv` and merges them.

---

## 6. Tips

* Add version numbers or change dates in the `Version/Rev` column when you swap parts.
* Use the `Notes` field to record why a specific part was chosen (e.g., "metal gears handle >2 kg·cm torque").
* Keep URLs up to date so future builders can order replacements quickly.

Happy tracking!

