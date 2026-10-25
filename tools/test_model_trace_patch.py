#!/usr/bin/env python3
"""Confere geração CMake e rejeição de SDK sem o ponto de instrumentação."""
from pathlib import Path
import subprocess
import sys
import tempfile
raiz=Path(__file__).resolve().parent.parent
sdk=Path(sys.argv[1]).resolve();cmake=sys.argv[2]
with tempfile.TemporaryDirectory() as t:
 p=Path(t);src=p/'src';src.mkdir()
 def projeto(onde,trace="ON",overrides="ON"):
  (src/'CMakeLists.txt').write_text(f'''cmake_minimum_required(VERSION 3.22)
project(TesteModelo NONE)
set(SDK "{onde.as_posix()}")
set(RUNTIME_SOURCES "${{SDK}}/runtime/kernel/files.cpp")
set(BLACK_TRACE_MODELS {trace})
set(BLACK_MODEL_OVERRIDES {overrides})
include("{(raiz/'cmake/ModelTrace.cmake').as_posix()}")
list(LENGTH RUNTIME_SOURCES n)
if(NOT n EQUAL 1)
 message(FATAL_ERROR "Fonte duplicada")
endif()
''')
 projeto(sdk)
 r=subprocess.run([cmake,'-S',str(src),'-B',str(p/'build')],capture_output=True,text=True)
 assert r.returncode==0,r.stdout+r.stderr
 patch=(p/'build/model_trace_files.cpp').read_text()
 assert patch.count('model_trace::Read(')==1 and 'model_trace::Wanted(f->trace_path)' in patch
 assert patch.count('opened->trace_path = ModelTracePath(name, root)')==2
 assert patch.count('f->trace_path = ModelTracePath(name, root)')==3
 assert 'parent->trace_path.empty()' in patch
 assert 'model_trace::Read(' not in (sdk/'runtime/kernel/files.cpp').read_text()
 projeto(sdk,trace='OFF',overrides='ON')
 r=subprocess.run([cmake,'-S',str(src),'-B',str(p/'override')],capture_output=True,text=True)
 assert r.returncode==0,r.stdout+r.stderr
 only=(p/'override/model_trace_files.cpp').read_text()
 assert 'model_override::Find' in only and 'model_trace::Read' not in only and 'trace_path' not in only
 projeto(sdk,trace='ON',overrides='OFF')
 r=subprocess.run([cmake,'-S',str(src),'-B',str(p/'trace')],capture_output=True,text=True)
 assert r.returncode==0,r.stdout+r.stderr
 only=(p/'trace/model_trace_files.cpp').read_text()
 assert 'model_override::Find' not in only and 'model_trace::Read' in only
 projeto(sdk,trace='OFF',overrides='OFF')
 r=subprocess.run([cmake,'-S',str(src),'-B',str(p/'off')],capture_output=True,text=True)
 assert r.returncode==0,r.stdout+r.stderr
 assert not (p/'off/model_trace_files.cpp').exists()
 falso=p/'sdk';(falso/'runtime/kernel').mkdir(parents=True)
 (falso/'runtime/kernel/files.cpp').write_text('// Layout desconhecido, sem Transfer\n')
 projeto(falso)
 r=subprocess.run([cmake,'-S',str(src),'-B',str(p/'invalido')],capture_output=True,text=True)
 assert r.returncode!=0 and 'SDK incompatível' in r.stderr
print('CMake: cópia instrumentada única, SDK original intacto e layout desconhecido rejeitado: OK')
