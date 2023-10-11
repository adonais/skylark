require("eu_core")

function run(pnode)
  local opt_file = (eu_core.script_path() .. "\\skylark_pretty.conf")
  if (not eu_core.file_exists(opt_file)) then
    local opt_code = -- xmlformater默认配置文件
    "-- xmlformater default configuration\n" ..
    "removal_comments = false\n" ..
    "removal_processing_instructions = false\n" ..
    "removal_cdata_sections = false\n" ..
    "removal_xml_declaration = false\n" ..
    "removal_doctype_declaration = false\n" ..
    "removal_character_data = false\n" ..
    "removal_attributes = false"
    eu_core.save_file(opt_file,  opt_code)
  end
  dofile(opt_file)
  local m_opt = eu_core.ffi.new("struct opt_format", {
      removal_comments,
      removal_processing_instructions,
      removal_cdata_sections,
      removal_xml_declaration,
      removal_doctype_declaration,
      removal_character_data,
      removal_attributes
  })
  if (not eu_core.euapi.eu_xml_pretty(pnode, m_opt)) then
    -- print("eu_xml_pretty return false")
    do return 1 end
  end
  return 0
end
