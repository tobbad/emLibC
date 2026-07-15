# ============================
# Projektname = Ordnername
# ============================
PROJECT := $(notdir $(CURDIR))

ZIP_SOURCES := $(shell find . -type f \( \
	-name "*.c" -o -name "*.h" -o -name "*.cpp" -o -name "*.cc" \
	-o -name "*.sh" -o -name "*.ioc" -o -name "*.ld"  \
	-o -name "*.cproject" -o -name "*.project" -o -name "*.mxproject" \
	-o -name "*.txt" -o -name "*.tex"  \
	-o -name ".gitmodules" -o -name ".gitignore" -o -name "Makefile" \
\) )

zip:
	@echo "ZIP_SOURCES = [$(ZIP_SOURCES)]"
	@if [ -z "$(ZIP_SOURCES)" ]; then \
		echo "Keine Dateien zum Packen gefunden"; \
		exit 1; \
	fi
	zip -r $(PROJECT).zip $(ZIP_SOURCES)
