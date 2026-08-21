#!/usr/bin/env python3
"""Deterministic policy checks for BSP, profiles, artifacts, and CI routing."""
from __future__ import annotations

import json
import importlib.util
import fnmatch
import re
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
MANAGED_BSP = 'waveshare/esp32_p4_wifi6_touch_lcd_x: "^2.0.3"'
MANIFESTS = (
    'examples/esp-idf/07_Displaycolorbar/main/idf_component.yml',
    'examples/esp-idf/08_lvgl_demo_v9/components/bsp_extra/idf_component.yml',
    'examples/esp-idf/09_video_lcd_display/main/idf_component.yml',
    'examples/esp-idf/10_mp4_player/main/idf_component.yml',
    'examples/esp-idf/11_esp_brookesia_phone/main/idf_component.yml',
    'examples/esp-idf/12_usb_extend_screen/components/bsp_extra/idf_component.yml',
)
ARDUINO_SKETCHES = (
    'examples/arduino/examples/01_DisplayColorBars/01_DisplayColorBars.ino',
    'examples/arduino/examples/02_TouchDrawing/02_TouchDrawing.ino',
    'examples/arduino/examples/03_AsciiTable/03_AsciiTable.ino',
    'examples/arduino/examples/04_LVGLV9/04_LVGLV9.ino',
    'examples/arduino/examples/05_WiFiAnalyzer/05_WiFiAnalyzer.ino',
    'examples/arduino/examples/06_CameraPreview/06_CameraPreview.ino',
    'examples/arduino/examples/07_CameraISPTuning/07_CameraISPTuning.ino',
    'examples/arduino/examples/08_SDCard/08_SDCard.ino',
    'examples/arduino/examples/09_AudioPlayback/09_AudioPlayback.ino',
    'examples/arduino/examples/10_MicRecord/10_MicRecord.ino',
)


class RepositoryPolicyTests(unittest.TestCase):
    def test_managed_bsp_version_and_no_local_copies(self) -> None:
        for relative in MANIFESTS:
            text = (ROOT / relative).read_text(encoding='utf-8')
            self.assertIn(MANAGED_BSP, text)
            self.assertNotIn('git: https://github.com/waveshareteam/Waveshare-ESP32-components.git', text)
            self.assertNotIn('path: bsp/esp32_p4_wifi6_touch_lcd_x', text)
        self.assertFalse(any(ROOT.glob('examples/esp-idf/*/components/esp32_p4_wifi6_touch_lcd_x/**/idf_component.yml')))

    def test_revision_overlays_and_rev3_example_defaults(self) -> None:
        rev1 = (ROOT / 'config/sdkconfig/rev1_3.defaults').read_text(encoding='utf-8')
        rev3 = (ROOT / 'config/sdkconfig/rev3_x.defaults').read_text(encoding='utf-8')
        self.assertIn('CONFIG_ESP32P4_SELECTS_REV_LESS_V3=y', rev1)
        self.assertIn('CONFIG_ESP32P4_REV_MIN_100=y', rev1)
        self.assertIn('# CONFIG_ESP32P4_SELECTS_REV_LESS_V3 is not set', rev3)
        self.assertIn('CONFIG_ESP32P4_REV_MIN_300=y', rev3)
        for setting in ('CONFIG_BOOTLOADER_LOG_LEVEL_ERROR=y', 'CONFIG_BOOTLOADER_LOG_LEVEL=1'):
            self.assertIn(setting, rev3)
            self.assertNotIn(setting, rev1)
        projects = sorted(
            path.parent
            for path in (ROOT / 'examples/esp-idf').glob('*/CMakeLists.txt')
            if (path.parent / 'main').is_dir()
        )
        self.assertEqual(len(projects), 12)
        for project in projects:
            defaults = (project / 'sdkconfig.defaults').read_text(encoding='utf-8')
            self.assertIn('# CONFIG_ESP32P4_SELECTS_REV_LESS_V3 is not set', defaults)
            self.assertIn('CONFIG_ESP32P4_REV_MIN_300=y', defaults)
            self.assertIn('CONFIG_BOOTLOADER_LOG_LEVEL_ERROR=y', defaults)
            self.assertIn('CONFIG_BOOTLOADER_LOG_LEVEL=1', defaults)
            self.assertNotIn('CONFIG_ESP32P4_SELECTS_REV_LESS_V3=y', defaults)
            self.assertNotRegex(defaults, r'CONFIG_ESP32P4_REV_MIN_(?:0|1|100)=y')
        target_defaults = '\n'.join(
            path.read_text(encoding='utf-8')
            for path in sorted((ROOT / 'examples/esp-idf').glob('*/sdkconfig.defaults.esp32p4'))
        )
        self.assertNotIn('CONFIG_ESP32P4_SELECTS_REV_LESS_V3=y', target_defaults)
        self.assertNotRegex(target_defaults, r'CONFIG_ESP32P4_REV_MIN_(?:0|1|100)=y')
        self.assertEqual(
            (ROOT / 'examples/esp-idf/11_esp_brookesia_phone/partitions.csv').read_text(encoding='utf-8'),
            '# Name,   Type, SubType, Offset,  Size, Flags\n'
            '# Note: if you have increased the bootloader size, make sure to update the offsets to avoid overlap\n'
            'nvs,      data, nvs,     ,         0x6000,\n'
            'phy_init, data, phy,     ,         0x1000,\n'
            'factory,  app,  factory, ,         15M,\n',
        )
        source = (ROOT / '.github/scripts/discover_esp_idf_examples.py').read_text(encoding='utf-8')
        self.assertIn('"revision_profile": "rev3_x"', source)
        self.assertIn('ARDUINO_ONLY_PATTERNS', source)
        self.assertIn('examples/arduino/**', source)
        for relative in ('docs/silicon-revisions.md', 'docs/silicon-revisions_ZH.md'):
            revision_docs = (ROOT / relative).read_text(encoding='utf-8')
            for required in ('CONFIG_ESP32P4_REV_MIN_100=y', 'CONFIG_ESP32P4_REV_MIN_300=y', 'PLL_F20M', 'XTAL'):
                self.assertIn(required, revision_docs)

    def test_arduino_surface_and_touch_ci_contract(self) -> None:
        self.assertEqual(tuple(sorted(ARDUINO_SKETCHES)), ARDUINO_SKETCHES)
        for relative in ARDUINO_SKETCHES:
            self.assertTrue((ROOT / relative).is_file())
        touch = '\n'.join(
            path.read_text(encoding='utf-8')
            for path in sorted((ROOT / 'examples/arduino/libraries/lcd_x/src').glob('lcd_x_board.*'))
        )
        self.assertIn('GPIO_NUM_NC', touch)
        self.assertIn('0x5D', touch)
        self.assertIn('0x14', touch)
        self.assertIn('config.device_address = address_', touch)
        self.assertNotIn('gpio_isr_handler_add', touch)
        workflow = (ROOT / '.github/workflows/arduino-examples.yml').read_text(encoding='utf-8')
        self.assertIn('ARDUINO_CLI_VERSION: "1.5.1"', workflow)
        self.assertIn('ARDUINO_CORE_VERSION: "3.3.11"', workflow)
        self.assertIn('compiler.cpp.extra_flags=-DLCD_X_DISPLAY_VARIANT=${{ matrix.display_value }}', workflow)
        self.assertNotIn('arduino-cli lib install', workflow)
        self.assertNotIn('LV_CONF_SKIP', workflow)
        self.assertNotIn('compiler.c.extra_flags=', workflow)
        self.assertNotIn('compiler.S.extra_flags=', workflow)
        self.assertIn('name: Arduino CI gate', workflow)

        lvgl_properties = (ROOT / 'examples/arduino/libraries/lvgl/library.properties').read_text(encoding='utf-8')
        self.assertRegex(lvgl_properties, r'(?m)^name=lvgl$')
        self.assertRegex(lvgl_properties, r'(?m)^version=9\.5\.0$')
        lv_conf = (ROOT / 'examples/arduino/libraries/lv_conf.h').read_text(encoding='utf-8')
        self.assertRegex(lv_conf, r'(?m)^#if\s+1\b.*enable')
        self.assertNotIn('LV_CONF_SKIP', lv_conf)

    def test_artifact_and_firmware_workflow_contract(self) -> None:
        packager = (ROOT / '.github/scripts/build_esp_idf_artifact.py').read_text(encoding='utf-8')
        for required in ('"schema_version": 2', '"flash_size_bytes": 32 * 1024 * 1024', 'revision_bounds', '"size"', 'merged_image', '--build-dir', '--sdkconfig', 'maintained-product-firmware', 'generated sdkconfig target is not esp32p4'):
            self.assertIn(required, packager)
        workflow = (ROOT / '.github/workflows/maintained-firmware.yml').read_text(encoding='utf-8')
        self.assertIn('revision_profile: [rev3_x]', workflow)
        self.assertEqual(workflow.count('revision_profile: [rev3_x]'), 1)
        self.assertEqual(workflow.count('name: Build maintained firmware (${{ matrix.revision_profile }})'), 1)
        self.assertIn('--revision-profile "${{ matrix.revision_profile }}"', workflow)
        self.assertIn('name: Maintained firmware gate', workflow)
        self.assertEqual(workflow.count('actions/upload-artifact@v7'), 3)
        self.assertIn('retention-days: 14', workflow)
        self.assertIn('id: metadata', workflow)
        self.assertIn('git rev-parse HEAD', workflow)
        self.assertIn('[[ "$sha" =~ ^[0-9a-f]{40}$ ]]', workflow)
        self.assertEqual(workflow.count('steps.metadata.outputs.sha12'), 6)
        self.assertEqual(workflow.count('if-no-files-found: error'), 3)
        self.assertIn('path: firmware/brookesia', workflow)
        self.assertIn('esp_idf_version: v5.5.5', workflow)
        self.assertNotIn('examples/esp-idf/11_esp_brookesia_phone', workflow)
        command = '''command: |
            set -euo pipefail
            for variant in lcd-7 lcd-8 lcd-10-1; do
              python ../../.github/scripts/build_esp_idf_artifact.py \\
                --repo-root ../.. \\
                --idf-version v5.5.5 \\
                --display-variant "$variant" \\
                --revision-profile "${{ matrix.revision_profile }}" \\
                --build-dir "build/ci-$variant" \\
                --sdkconfig "sdkconfig.ci-$variant" \\
                --package-artifact true \\
                --artifact-kind maintained-product-firmware \\
                --expected-checkout-sha "${{ github.event.pull_request.head.sha || github.sha }}"
            done'''
        self.assertIn(command, workflow)
        self.assertNotIn('command: >-', workflow)

    def test_routing_config_is_narrow_and_matches_discovery_contract(self) -> None:
        config = json.loads((ROOT / '.github/ci-routing-config.json').read_text(encoding='utf-8'))
        self.assertEqual(config['documentation_asset_patterns'], [
            'assets/ESP32-P4-WIFI6-Touch-LCD-X.jpg',
            'schematic/*.pdf',
            'examples/esp-idf/11_esp_brookesia_phone/components/brookesia_core/docs/**/*.gif',
            'examples/esp-idf/11_esp_brookesia_phone/components/brookesia_core/docs/**/*.png',
            'examples/esp-idf/11_esp_brookesia_phone/components/brookesia_core/systems/speaker/docs/**/*.gif',
            'examples/esp-idf/11_esp_brookesia_phone/components/brookesia_core/systems/speaker/docs/**/*.png',
        ])
        self.assertEqual(config['ignore_build_patterns'], [
            '.gitattributes',
            '.github/markdown-audit-config.json',
            '.github/scripts/audit_markdown.py',
            '.github/scripts/check_markdown_links.py',
            '.github/scripts/test_audit_markdown.py',
            '.github/scripts/test_check_markdown_links.py',
            '.github/workflows/docs.yml',
            'Flash-CI-Firmware.cmd',
            'scripts/Flash-CI-Firmware.ps1',
            'examples/esp-idf/**/LICENSE*',
            'examples/esp-idf/**/license*',
        ])
        self.assertEqual(config['firmware_patterns'], ['firmware/brookesia/**'])
        self.assertEqual(config['arduino_shared_patterns'], ['examples/arduino/libraries/**'])
        self.assertEqual(config['arduino_global_patterns'], [
            '.github/scripts/discover_arduino_examples.py',
            '.github/scripts/test_discover_arduino_examples.py',
            '.github/scripts/test_repository_policy.py',
            '.github/workflows/arduino-examples.yml',
            '.github/ci-routing-config.json',
            '.gitignore',
        ])
        self.assertEqual(config['esp_idf_global_patterns'], [
            '.github/scripts/build_esp_idf_artifact.py',
            '.github/scripts/discover_esp_idf_examples.py',
            '.github/scripts/test_build_esp_idf_artifact.py',
            '.github/scripts/test_discover_esp_idf_examples.py',
            '.github/scripts/test_repository_policy.py',
            '.github/workflows/esp-idf-examples.yml',
            '.github/workflows/maintained-firmware.yml',
            '.github/ci-routing-config.json',
            'config/display-variants.json',
            '.gitignore',
        ])

        module_path = ROOT / '.github/scripts/discover_esp_idf_examples.py'
        spec = importlib.util.spec_from_file_location('routing_discovery', module_path)
        self.assertIsNotNone(spec)
        self.assertIsNotNone(spec.loader)
        discovery = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(discovery)
        examples = set(discovery.list_examples())
        manifest = discovery.load_display_manifest()

        def matches(key: str, path: str) -> bool:
            return any(
                fnmatch.fnmatchcase(path, pattern)
                for pattern in config[key]
            )

        def matches_effective_esp_idf_global(path: str) -> bool:
            default_patterns = (
                'CMakeLists.txt',
                'idf_component.yml',
                'sdkconfig*',
                'partitions*.csv',
                'config/*.cmake',
                'config/**/*.cmake',
                'config/*.defaults',
                'config/**/*.defaults',
                'config/*.h',
                'config/**/*.h',
            )
            return matches('esp_idf_global_patterns', path) or any(
                fnmatch.fnmatchcase(path, pattern)
                for pattern in default_patterns
            )

        configured_lightweight = {
            path.lower() for path in config['ignore_build_patterns']
        }
        self.assertTrue(
            discovery.LIGHTWEIGHT_ONLY_PATHS <= configured_lightweight
        )
        self.assertTrue(
            discovery.LIGHTWEIGHT_ONLY_PATHS.isdisjoint(
                path.lower() for path in config['esp_idf_global_patterns']
            )
        )

        no_build_paths = (
            '.gitattributes',
            '.github/markdown-audit-config.json',
            '.github/scripts/audit_markdown.py',
            '.github/scripts/check_markdown_links.py',
            '.github/scripts/test_audit_markdown.py',
            '.github/scripts/test_check_markdown_links.py',
            '.github/workflows/docs.yml',
            'Flash-CI-Firmware.cmd',
            'scripts/Flash-CI-Firmware.ps1',
            'assets/ESP32-P4-WIFI6-Touch-LCD-X.jpg',
            'schematic/ESP32-P4-WIFI6-Touch-LCD-X-Schematic.pdf',
            'examples/esp-idf/08_lvgl_demo_v9/components/bsp_extra/LICENSE',
            'examples/esp-idf/11_esp_brookesia_phone/components/brookesia_core/license.txt',
            'examples/esp-idf/11_esp_brookesia_phone/components/brookesia_core/docs/_static/readme/app_launcher_demo.png',
            'examples/esp-idf/11_esp_brookesia_phone/components/brookesia_core/systems/speaker/docs/assets/animation/boot/boot_animation_360_360.gif',
        )
        for path in no_build_paths:
            with self.subTest(path=path):
                self.assertTrue(
                    matches('ignore_build_patterns', path)
                    or matches('documentation_asset_patterns', path)
                )
                selected, _ = discovery.discover_selection_from_paths(
                    [path], examples, manifest
                )
                self.assertEqual(selected, [])

        for path in ('firmware/ESP32-P4-WIFI6-Touch-LCD-7-FactoryOnly.bin', 'firmware/brookesia/README.md'):
            with self.subTest(path=path):
                self.assertFalse(discovery.firmware_build_impact([path]))
        for path in ('firmware/brookesia/main/app_main.c', 'firmware/brookesia/CMakeLists.txt', 'firmware/brookesia/assets/startup.dat'):
            with self.subTest(path=path):
                self.assertTrue(matches('firmware_patterns', path))
                self.assertTrue(discovery.firmware_build_impact([path]))

        for path in ('config/display-variants.json', '.gitignore'):
            with self.subTest(path=path):
                self.assertTrue(matches('esp_idf_global_patterns', path))
                selected, variant = discovery.discover_selection_from_paths(
                    [path], examples, manifest
                )
                self.assertEqual(set(selected), examples)
                self.assertEqual(
                    len(discovery.build_matrix(selected, manifest, variant)['include']),
                    48,
                )

        display_examples = set(manifest['display_examples'])
        for variant in manifest['variants']:
            path = variant['overlay']
            with self.subTest(overlay=path):
                self.assertTrue(matches_effective_esp_idf_global(path))
                selected, routed_variant = discovery.discover_selection_from_paths(
                    [path], examples, manifest
                )
                self.assertEqual(set(selected), display_examples)
                self.assertLess(set(selected), examples)
                self.assertEqual(routed_variant, variant['slug'])
                self.assertEqual(
                    len(
                        discovery.build_matrix(
                            selected, manifest, routed_variant
                        )['include']
                    ),
                    12,
                )

    def test_ci_firmware_gui_binds_final_selected_port(self) -> None:
        flasher = (ROOT / 'scripts/Flash-CI-Firmware.ps1').read_text(encoding='utf-8')
        self.assertIn('$portBox.Text=$Port', flasher)
        self.assertNotIn('$portBox.Text=$state.Port', flasher)
        self.assertIn("[string]$Profile = 'rev3_x'", flasher)
        self.assertIn("if ($Profile -ne 'rev3_x')", flasher)
        self.assertIn('inventory=21 (18 example rev3_x artifacts; 3 maintained rev3_x artifacts)', flasher)
        self.assertIn("$Script:ExampleIdfVersion = 'v6.0.2'", flasher)
        self.assertIn("$Script:MaintainedIdfVersion = 'v5.5.5'", flasher)
        self.assertIn("Example = 'firmware/brookesia'", flasher)
        self.assertIn("ExampleName = 'brookesia'", flasher)
        self.assertIn('$manifest.idf_version -eq $Item.IdfVersion', flasher)
        self.assertIn("'-brookesia-rev3_x-5\\.5\\.5-[a-f0-9]{12}$'", flasher)
        self.assertNotIn("Example = 'examples/esp-idf/11_esp_brookesia_phone'; ExampleName = '11_esp_brookesia_phone';", flasher)

    def test_workflow_routing_wires_discovery_outputs_to_consumers(self) -> None:
        docs = (ROOT / '.github/workflows/docs.yml').read_text(encoding='utf-8')
        examples = (ROOT / '.github/workflows/esp-idf-examples.yml').read_text(encoding='utf-8')
        firmware = (ROOT / '.github/workflows/maintained-firmware.yml').read_text(encoding='utf-8')
        arduino = (ROOT / '.github/workflows/arduino-examples.yml').read_text(encoding='utf-8')

        for workflow in (docs, examples, firmware, arduino):
            self.assertIn('cancel-in-progress: true', workflow)

        scope = re.search(
            r'- name: Classify changed scope\n(?P<body>.*?)\n      - name: Audit Markdown policy and diff scope',
            docs,
            re.DOTALL,
        )
        self.assertIsNotNone(scope)
        self.assertRegex(
            scope.group('body'),
            r'discover_esp_idf_examples\.py\s+\\\n\s*--base-ref "\$base_ref"\s+\\\n\s*--head-ref "\$head_ref"',
        )
        self.assertIn('DOCS_ONLY: ${{ steps.scope.outputs.docs_only }}', docs)
        self.assertRegex(
            docs,
            r'if \[\[ "\$DOCS_ONLY" == "true" \]\]; then\s+"\$\{audit\[@\]\}" --base "\$BASE_REF" --expect-docs-only --strict',
        )

        self.assertRegex(
            examples,
            r'elif \[\[ "\$\{\{ github\.event_name \}\}" == "pull_request" \]\]; then\s+python .*discover_esp_idf_examples\.py\s+\\\n\s*--base-ref "\$\{\{ github\.event\.pull_request\.base\.sha \}\}"\s+\\\n\s*--head-ref "\$\{\{ github\.event\.pull_request\.head\.sha \}\}"',
        )
        self.assertIn('matrix: ${{ steps.examples.outputs.matrix }}', examples)
        self.assertIn('has_examples: ${{ steps.examples.outputs.has_examples }}', examples)
        self.assertIn("if: needs.discover.outputs.has_examples == 'true'", examples)
        self.assertIn('matrix: ${{ fromJson(needs.discover.outputs.matrix) }}', examples)

        self.assertIn('build: ${{ steps.scope.outputs.firmware_build_impact }}', firmware)
        self.assertRegex(
            firmware,
            r'discover_esp_idf_examples\.py --base-ref "\$\{\{ github\.event\.pull_request\.base\.sha \}\}" --head-ref "\$\{\{ github\.event\.pull_request\.head\.sha \}\}"',
        )
        self.assertIn("if: needs.discover.outputs.build == 'true'", firmware)

        self.assertIn('discover_arduino_examples.py', arduino)
        self.assertIn('matrix: ${{ fromJson(needs.discover.outputs.matrix) }}', arduino)
        self.assertIn("if: needs.discover.outputs.has_examples == 'true'", arduino)
        self.assertIn('compiler.cpp.extra_flags=-DLCD_X_DISPLAY_VARIANT=${{ matrix.display_value }}', arduino)
        self.assertNotIn('arduino-cli lib install', arduino)
        self.assertNotIn('LV_CONF_SKIP', arduino)
        self.assertIn('name: Arduino CI gate', arduino)

    def test_discovery_matrix_and_maintained_firmware_combinations(self) -> None:
        module_path = ROOT / '.github/scripts/discover_esp_idf_examples.py'
        spec = importlib.util.spec_from_file_location('discover_esp_idf_examples', module_path)
        self.assertIsNotNone(spec)
        self.assertIsNotNone(spec.loader)
        module = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(module)
        matrix = module.build_matrix(module.list_examples())['include']
        self.assertEqual(len(matrix), 48)
        self.assertEqual(sum(row['package_artifact'] == 'true' for row in matrix), 18)

        arduino_path = ROOT / '.github/scripts/discover_arduino_examples.py'
        arduino_spec = importlib.util.spec_from_file_location('discover_arduino_examples', arduino_path)
        self.assertIsNotNone(arduino_spec)
        self.assertIsNotNone(arduino_spec.loader)
        arduino_module = importlib.util.module_from_spec(arduino_spec)
        arduino_spec.loader.exec_module(arduino_module)
        arduino_matrix = arduino_module.build_matrix(arduino_module.list_examples())['include']
        self.assertEqual(len(arduino_matrix), 30)

        firmware = (ROOT / '.github/workflows/maintained-firmware.yml').read_text(encoding='utf-8')
        profiles = re.search(r'revision_profile: \[([^]]+)\]', firmware)
        products = re.search(r'for variant in ([^;]+); do', firmware)
        self.assertIsNotNone(profiles)
        self.assertIsNotNone(products)
        profile_values = tuple(item.strip() for item in profiles.group(1).split(','))
        product_values = tuple(products.group(1).split())
        combinations = {(profile, product) for profile in profile_values for product in product_values}
        self.assertEqual(combinations, {('rev3_x', 'lcd-7'), ('rev3_x', 'lcd-8'), ('rev3_x', 'lcd-10-1')})


if __name__ == '__main__':
    unittest.main(verbosity=2)
