#!/usr/bin/env python3
from weasyprint import HTML

HTML("test_links.html").write_pdf("test_links.pdf")
print("Test PDF generated successfully")
