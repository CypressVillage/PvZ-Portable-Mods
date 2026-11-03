"""Run plain-plant contracts and native integration on an existing Linux/Ninja build.

The integration executable replaces only main.cpp, using the exact game objects
and linker flags from the build. Nothing is installed or enabled in real userdata.
"""
import argparse
import json
import os
from pathlib import Path
import shlex
import shutil
import subprocess
import tempfile


def run(args, **kwargs):
    subprocess.run([str(arg) for arg in args], check=True, **kwargs)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build", type=Path, required=True)
    parser.add_argument("--resources", type=Path,
                        help="optional game resource directory with main.pak and properties/")
    args = parser.parse_args()
    repo = Path(__file__).resolve().parent.parent
    build = args.build.resolve()
    output = Path(tempfile.mkdtemp(prefix="plain-plants-", dir="/tmp/opencode"))
    print(f"Test artifacts: {output}", flush=True)
    run(["cmake", "--build", build, "--target", "pvz-portable", "-j", "4"])
    commands = json.loads((build / "compile_commands.json").read_text())
    main_command = next(item for item in commands if Path(item["file"]).resolve() == repo / "src/main.cpp")
    original = main_command.get("arguments") or shlex.split(main_command["command"])

    def compile_test(source, obj):
        command = list(original)
        command[command.index("-o") + 1] = str(obj)
        command[command.index("-c") + 1] = str(source)
        run(command, cwd=main_command["directory"])

    runtime_object = output / "runtime.o"
    compile_test(repo / "tools/test-plain-plants.cpp", runtime_object)
    runtime_binary = output / "test-runtime"
    run([original[0], runtime_object, build / "libpvz_mod.a", build / "libpvz_lua.a", "-lm", "-o", runtime_binary])
    run([runtime_binary, repo, output / "fixtures"])
    if not args.resources:
        return

    resources = args.resources.resolve()
    if not (resources / "main.pak").is_file() or not (resources / "properties").is_dir():
        raise RuntimeError("resources must contain main.pak and properties/")
    resource_root = output / "resources"
    resource_root.mkdir()
    (resource_root / "main.pak").symlink_to(resources / "main.pak")
    (resource_root / "properties").symlink_to(resources / "properties", target_is_directory=True)
    shutil.copytree(repo / "examples/mods/plain-plant", resource_root / "mods/plain-plant")
    with (resource_root / "mods/plain-plant/main.lua").open("a") as source:
        source.write('''
for i=1,41 do
    plants.define("zz" .. i, {appearance={image="assets/stump.png"}})
end
hooks.on("plant.before_initialize", {type="plain-plant:stump"}, function(ctx) ctx.health=600 end)
hooks.on("plant.before_initialize", {type="base:peashooter"}, function(ctx) ctx.health=321 end)
hooks.on("plant.can_place", {type="plain-plant:stump"}, function(ctx) ctx.cancel=ctx.column==0 end)
''')
    save_root = output / "saves"
    (save_root / "userdata").mkdir(parents=True)
    (save_root / "userdata/mods.json").write_text('{"enabled":["plain-plant"]}')

    game_object = output / "game-test.o"
    compile_test(repo / "tools/test-plain-plants-game.cpp", game_object)
    link_lines = subprocess.check_output(["ninja", "-t", "commands", "pvz-portable"], cwd=build, text=True)
    line = next(line for line in reversed(link_lines.splitlines())
                if " -o pvz-portable " in line and "src/main.cpp.o" in line)
    segment = next(segment for segment in line.split(" && ") if "src/main.cpp.o" in segment)
    link = shlex.split(segment)
    link = [str(game_object) if arg.endswith("/src/main.cpp.o") else arg for arg in link]
    game_binary = output / "test-game"
    link[link.index("-o") + 1] = str(game_binary)
    run(link, cwd=build)
    def game_test(savedir, label):
        log_path = output / f"{label}.log"
        with log_path.open("w") as log:
            try:
                run(["xvfb-run", "-a", game_binary, f"-resdir={resource_root}", f"-savedir={savedir}"],
                    timeout=120, env={**os.environ, "SDL_AUDIODRIVER": "dummy"},
                    stdout=log, stderr=subprocess.STDOUT, cwd=output)
            except (subprocess.CalledProcessError, subprocess.TimeoutExpired):
                print("\n".join(log_path.read_text().splitlines()[-40:]), flush=True)
                raise
        print(f"{label}: passed; log: {log_path}", flush=True)

    game_test(save_root, "modded")
    assert not list((save_root / "userdata").glob("*.dat")), "vanilla profile was written"
    assert not list(save_root.rglob("must-not-exist.v4")), "unsafe save was written"
    baseline_root = output / "baseline-saves"
    baseline_root.mkdir()
    game_test(baseline_root, "vanilla")


if __name__ == "__main__":
    main()
