import sys, os

gs_templates_folder= '/usr/local/share/gs_templates'

build_project_name = 'csp-client'
project_name = u'CSP Client'
filename_pdf = u'gs-man-csp-client-'

copyright_name = u'2016, GomSpace'
company_name = u'GomSpace'

front_name = "GomSpace"
front_doc_type = "Manual"
front_doc_category = "Software Documentation"
front_image = "csp_client_front"

execfile(os.path.join(os.path.abspath(gs_templates_folder), 'conf_template.py'))

exclude_patterns = ['clients', 'lib/libcsp/**', 'lib/libparam/**', 'lib/liblog/**', 'lib/libgosh/**', 'lib/libftp/**', 'lib/libutil/**', 'lib/libhk/**', 'lib/libfp/**']
