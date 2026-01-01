import PyPDF2
import sys

pdf_file = sys.argv[1]
with open(pdf_file, 'rb') as f:
    reader = PyPDF2.PdfReader(f)
    print(f"Pages: {len(reader.pages)}")
