#!/usr/bin/env python3
"""
Generador de archivos MP3 placeholder para validación de audio
Genera archivos placeholder (vacíos) para los tracks 39-68 que faltan
"""

import os
import sys

def create_placeholder_mp3s(start_track=39, end_track=68, output_dir="audio"):
    """
    Crea archivos MP3 placeholder para los tracks especificados
    
    Args:
        start_track: Primer track a generar (por defecto 39)
        end_track: Último track a generar (por defecto 68)
        output_dir: Directorio de salida (por defecto "audio")
    """
    
    # Crear directorio si no existe
    if not os.path.exists(output_dir):
        os.makedirs(output_dir)
        print(f"📁 Directorio '{output_dir}' creado")
    
    print(f"\n🎵 Generando archivos MP3 placeholder ({start_track:04d}-{end_track:04d})...")
    print(f"📂 Directorio: {output_dir}")
    print("-" * 60)
    
    created_count = 0
    skipped_count = 0
    
    for track_num in range(start_track, end_track + 1):
        filename = f"{track_num:04d}.mp3"
        filepath = os.path.join(output_dir, filename)
        
        if os.path.exists(filepath):
            print(f"⏭️  {filename} - Ya existe, omitiendo")
            skipped_count += 1
        else:
            # Crear archivo vacío (placeholder)
            with open(filepath, 'wb') as f:
                # Escribir 0 bytes para archivos placeholder
                pass
            print(f"✅ {filename} - Creado")
            created_count += 1
    
    print("-" * 60)
    print(f"\n📊 Resumen:")
    print(f"  ✅ Archivos creados: {created_count}")
    print(f"  ⏭️  Archivos omitidos (ya existían): {skipped_count}")
    print(f"  📦 Total procesado: {created_count + skipped_count}")
    
    print(f"\n⚠️  IMPORTANTE:")
    print(f"  Los archivos creados son PLACEHOLDERS (0 bytes)")
    print(f"  Debes reemplazarlos con archivos MP3 reales generados con:")
    print(f"    - TTSMaker.com (recomendado)")
    print(f"    - gTTS (Python)")
    print(f"    - Natural Readers")
    print(f"  Ver docs/AUDIO_TRACKS_GUIDE.md para instrucciones completas")
    
    return created_count, skipped_count

def validate_all_tracks(track_count=68, audio_dir="audio"):
    """
    Valida que todos los tracks estén presentes
    
    Args:
        track_count: Número total de tracks esperados
        audio_dir: Directorio de audio
    """
    print(f"\n🔍 Validando presencia de {track_count} tracks en '{audio_dir}'...")
    print("-" * 60)
    
    missing_tracks = []
    empty_tracks = []
    valid_tracks = []
    
    for track_num in range(1, track_count + 1):
        filename = f"{track_num:04d}.mp3"
        filepath = os.path.join(audio_dir, filename)
        
        if not os.path.exists(filepath):
            missing_tracks.append(track_num)
            print(f"❌ Track {track_num:04d}.mp3 - FALTANTE")
        else:
            file_size = os.path.getsize(filepath)
            if file_size == 0:
                empty_tracks.append(track_num)
                print(f"⚠️  Track {track_num:04d}.mp3 - PLACEHOLDER (0 bytes)")
            else:
                valid_tracks.append(track_num)
                print(f"✅ Track {track_num:04d}.mp3 - OK ({file_size} bytes)")
    
    print("-" * 60)
    print(f"\n📊 Resumen de Validación:")
    print(f"  ✅ Tracks válidos (con contenido): {len(valid_tracks)}")
    print(f"  ⚠️  Tracks placeholder (0 bytes): {len(empty_tracks)}")
    print(f"  ❌ Tracks faltantes: {len(missing_tracks)}")
    print(f"  📦 Total tracks esperados: {track_count}")
    
    if missing_tracks:
        print(f"\n❌ Tracks faltantes: {missing_tracks}")
    
    if empty_tracks:
        print(f"\n⚠️  Tracks placeholder que necesitan MP3 real:")
        print(f"  {empty_tracks}")
    
    all_present = len(missing_tracks) == 0
    all_valid = len(missing_tracks) == 0 and len(empty_tracks) == 0
    
    if all_valid:
        print(f"\n✅ ¡VALIDACIÓN EXITOSA! Todos los {track_count} tracks están presentes y tienen contenido")
    elif all_present:
        print(f"\n⚠️  VALIDACIÓN PARCIAL: Todos los tracks están presentes pero {len(empty_tracks)} son placeholders")
        print(f"   Genera los archivos MP3 reales antes de usar el sistema")
    else:
        print(f"\n❌ VALIDACIÓN FALLIDA: Faltan {len(missing_tracks)} tracks")
    
    return all_present, all_valid

def main():
    """Función principal"""
    print("=" * 60)
    print("🎵 Validador de Audio Tracks - Sistema de Coche Marcos")
    print("=" * 60)
    
    # Determinar directorio de audio
    script_dir = os.path.dirname(os.path.abspath(__file__))
    audio_dir = os.path.join(script_dir, "audio")
    
    # Verificar si estamos en el directorio correcto
    if not os.path.exists(audio_dir):
        # Intentar directorio padre
        parent_audio = os.path.join(os.path.dirname(script_dir), "audio")
        if os.path.exists(parent_audio):
            audio_dir = parent_audio
        else:
            print(f"⚠️  Advertencia: Directorio 'audio' no encontrado")
            print(f"   Se creará: {audio_dir}")
    
    if len(sys.argv) > 1:
        if sys.argv[1] == "validate":
            # Solo validar
            validate_all_tracks(68, audio_dir)
        elif sys.argv[1] == "generate":
            # Generar placeholders para tracks 39-68
            create_placeholder_mp3s(39, 68, audio_dir)
            print("\n")
            validate_all_tracks(68, audio_dir)
        elif sys.argv[1] == "generate-all":
            # Generar placeholders para todos los tracks
            create_placeholder_mp3s(1, 68, audio_dir)
            print("\n")
            validate_all_tracks(68, audio_dir)
        else:
            print(f"❌ Comando desconocido: {sys.argv[1]}")
            print(f"\nUso:")
            print(f"  {sys.argv[0]} validate       - Validar tracks existentes")
            print(f"  {sys.argv[0]} generate       - Generar placeholders (39-68)")
            print(f"  {sys.argv[0]} generate-all   - Generar placeholders (1-68)")
    else:
        # Comportamiento por defecto: generar tracks 39-68 y validar
        print("\n📋 Modo por defecto: Generar tracks 39-68 + Validar")
        create_placeholder_mp3s(39, 68, audio_dir)
        print("\n")
        validate_all_tracks(68, audio_dir)
        
        print(f"\n💡 Tip: Usa '{sys.argv[0]} validate' para solo validar")

if __name__ == "__main__":
    main()
