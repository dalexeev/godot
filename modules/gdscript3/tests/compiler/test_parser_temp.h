#pragma once

#include "../../compiler/parser.h"
#include "print_utils.h"

#include "core/io/dir_access.h"
#include "core/io/file_access.h"
#include "tests/test_macros.h"

namespace gdscript {

void test_directory(const String &p_path) {
	const Ref<DirAccess> dir = DirAccess::open(p_path);
	ERR_FAIL_COND(dir.is_null());

	for (const String &subdir_name : dir->get_directories()) {
		test_directory(p_path.path_join(subdir_name));
	}

	const String primary_separator = String::chr('=').repeat(80);
	const String secondary_separator = String::chr('-').repeat(80);

	for (const String &file_name : dir->get_files()) {
		if (file_name.get_extension() == "out") {
			const Ref<FileAccess> out_file = FileAccess::open(p_path.path_join(file_name), FileAccess::READ);
			ERR_CONTINUE(out_file.is_null());

			if (out_file->get_line().begins_with("GDTEST_PARSER_ERROR")) {
				continue;
			}

			const String gd_file_name = file_name.get_basename() + ".gd";
			const String gd_file_path = p_path.path_join(gd_file_name);

			const Ref<FileAccess> gd_file = FileAccess::open(gd_file_path, FileAccess::READ);
			ERR_CONTINUE(gd_file.is_null());

			String gd_source = gd_file->get_as_text();
			gd_source = gd_source.replace("static func ", "@static func ");
			gd_source = gd_source.replace("static var ", "@static var ");
			gd_source = gd_source.replace("@static_unload", "");

			Parser parser(gd_file_path, gd_source);
			parser.parse();

			const List<DiagnosticList::Error> &errors = parser.get_ast().get_diagnostic_list().get_errors();
			if (!errors.is_empty()) {
				print_line(primary_separator);
				print_line(gd_file_path);

				const Vector<String> lines = gd_source.split("\n");
				for (const DiagnosticList::Error &error : errors) {
					print_line(secondary_separator);
					print_source_region(error.source_region, lines);
					print_line(vformat(" --> Error: %s", error.message));
				}
			}
		}
	}
}

TEST_CASE("[Modules][GDScript3][Compiler][Parser] Temp test") {
	test_directory("./modules/gdscript/tests/scripts/analyzer/");
	test_directory("./modules/gdscript/tests/scripts/parser/");
	test_directory("./modules/gdscript/tests/scripts/runtime/");
}

} // namespace gdscript
