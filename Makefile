.PHONY: bom format docs

bom:
	@echo "Aggregating bills of materials..."
	@python3 tools/bom_aggregate.py

format:
	@echo "No automatic formatter configured."

docs:
	@echo "Open README.md for documentation index."
