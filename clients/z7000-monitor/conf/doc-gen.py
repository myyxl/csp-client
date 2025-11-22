#!/usr/bin/env python

import json

datatypes = {"uint8": ["UINT8", 1], "uint16": ["UINT16", 2], "uint32": ["UINT32", 4], "uint64": ["UINT64", 8], 
           "int8": ["INT8", 1], "int16": ["INT16", 2], "int32": ["INT32", 4], "int64": ["INT64", 8], 
             "x8": ["X8", 1], "x16": ["X16", 2], "x32": ["X32", 4], "x64": ["X64", 8],
             "double": ["DOUBLE", 8], "float": ["FLOAT", 4], "string": ["STRING", -1], "data": ["DATA", -1], "bool": ["BOOL", 1]}

with open('tables/soft_tables.json') as fd:
    data = json.load(fd)

with open('params.rst', 'w+') as fd:
    fd.write(" Parameters \n")
    fd.write("*************************\n\n")
    for table in data['tables']:
        fd.write(".. tabularcolumns:: |p{1.8cm}|p{1.0cm}|p{2cm}|p{2cm}|p{2cm}|p{0.5cm}|p{5cm}|\n")
        fd.write(".. csv-table:: " + table['name'] + '\n')
        fd.write("   :header: \"Name\", \"type\", \"default value\", \"min\", \"max\", \"unit\", \"description\"\n")
        fd.write("\n")

        for par in table['parameters']:
            fd.write('   \"' + par['name'] + '\", \"' + par['type'] + ' ' + par.get('modifier', '') + '\", \"' + str(par.get('default', '-')) + '\", \"' + str(par.get('min', '')) + '\", \"' + str(par.get('max', '')) + '\", \"' + str(par.get('unit', '')) + '\", \"' + par.get('description', '') + '\"\n')

        fd.write("\n\n")
