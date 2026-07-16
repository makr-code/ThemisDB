import sys
sys.path.insert(0, 'tools')
from gs3_base_scanner import ScannerRegistry, GapScannerPipeline
sys.path.insert(0, 'tools/scanners')
from gs3_step04_design_wrapper_abstraction_excess import WrapperAbstractionExcessScanner

reg = ScannerRegistry()
reg.register(WrapperAbstractionExcessScanner())
pipe = GapScannerPipeline(reg)
pipe.file_centric_mode = True
pipe.include_graph_path = 'ai_working/include_graph_tools_scanners_libclang.json'

print('--- START PoC file-centric run with graph (tools/scanners)')
res = pipe.execute('tools/scanners', verbose=True)
print('--- PoC complete. Gaps found:', len(res))
