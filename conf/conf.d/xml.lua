xml = {}

function xml.get_tabattr()
  -- 0, 跟随主配置
  local tab_width = 0
  -- 0, 不转换 1, 转换, -1, 跟随主配置
  local tab_convert_spaces = 1
  return tab_width,tab_convert_spaces
end

function xml.create_bakup(path)
  local xml_code = {
    "user_xml = {}\n",
    "function user_xml.get_tabattr()\n",
    "  -- 0, 跟随主配置\n",
    "  local tab_width = 0\n",
    "  -- 0, 不转换 1, 转换, -1, 跟随主配置\n",
    "  local tab_convert_spaces = 1\n",
    "  return tab_width,tab_convert_spaces\n",
    "end\n",
    "return user_xml",
  }
  local shell_code = table.concat(xml_code)
  eu_core.save_file(path, shell_code)
  shell_code = nil
  xml_code = nil
end

return xml