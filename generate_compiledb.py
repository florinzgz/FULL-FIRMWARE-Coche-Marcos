Import("env")
import json
from pathlib import Path

def generate_compile_commands(*args, **kwargs):
    """Genera compile_commands.json para el entorno actual"""
    
    build_dir = env.subst("$BUILD_DIR")
    project_dir = env.subst("$PROJECT_DIR")
    
    # Crear estructura básica
    compile_commands = []
    
    # Obtener configuración del compilador
    cc = env.subst("$CC")
    cxx = env.subst("$CXX")
    cppdefines = env.get("CPPDEFINES", [])
    cpppath = env.get("CPPPATH", [])
    ccflags = env.get("CCFLAGS", [])
    cxxflags = env.get("CXXFLAGS", [])
    
    # Asegurar que las flags sean listas
    if not isinstance(ccflags, list):
        ccflags = [ccflags] if ccflags else []
    if not isinstance(cxxflags, list):
        cxxflags = [cxxflags] if cxxflags else []
    
    # Convertir defines
    defines = []
    for define in cppdefines:
        if isinstance(define, tuple):
            defines.append(f"-D{define[0]}={define[1]}")
        else:
            defines.append(f"-D{define}")
    
    # Convertir includes
    includes = [f"-I{path}" for path in cpppath]
    
    # Procesar archivos fuente
    src_dir = Path(project_dir) / "src"
    
    for src_file in src_dir.rglob("*"):
        if src_file.suffix in [".c", ".cpp", ".cc", ".cxx"]:
            compiler = cxx if src_file.suffix in [".cpp", ".cc", ".cxx"] else cc
            flags = cxxflags if src_file.suffix in [".cpp", ".cc", ".cxx"] else ccflags
            
            command = [compiler] + flags + defines + includes + ["-c", str(src_file)]
            
            compile_commands.append({
                "directory": project_dir,
                "command": " ".join(str(x) for x in command),
                "file": str(src_file.relative_to(project_dir))
            })
    
    # Guardar archivo
    output_file = Path(build_dir) / "compile_commands.json"
    output_file.parent.mkdir(parents=True, exist_ok=True)
    
    with open(output_file, "w") as f:
        json.dump(compile_commands, f, indent=2)
    
    print(f"✓ Generated compile_commands.json at {output_file}")
    print(f"  Total entries: {len(compile_commands)}")

# Registrar callback después de la compilación
env.AddPostAction("$BUILD_DIR/${PROGNAME}.elf", generate_compile_commands)
