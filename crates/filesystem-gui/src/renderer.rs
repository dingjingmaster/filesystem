use iced::wgpu;
use std::borrow::Cow;
use std::env;
use std::panic::{self, AssertUnwindSafe};

const ICED_BACKEND: &str = "ICED_BACKEND";
const TINY_SKIA_BACKEND: &str = "tiny-skia";

const WGPU_PROBE_SHADER: &str = r#"
struct VertexOutput {
    @builtin(position) position: vec4<f32>,
};

@vertex
fn vs_main(@builtin(vertex_index) index: u32) -> VertexOutput {
    var positions = array<vec2<f32>, 3>(
        vec2<f32>(-1.0, -1.0),
        vec2<f32>(3.0, -1.0),
        vec2<f32>(-1.0, 3.0),
    );

    var output: VertexOutput;
    output.position = vec4<f32>(positions[index], 0.0, 1.0);
    return output;
}

@fragment
fn fs_main() -> @location(0) vec4<f32> {
    let unpacked = unpack2x16float(0x3c003c00u);
    return vec4<f32>(unpacked.x, unpacked.y, 0.0, 1.0);
}
"#;

pub(crate) fn configure_backend() {
    if let Some(selected) = selected_backend(env::var(ICED_BACKEND).ok().as_deref(), wgpu_supported)
    {
        env::set_var(ICED_BACKEND, selected);
    }
}

fn selected_backend(
    configured_backend: Option<&str>,
    probe: impl FnOnce() -> bool,
) -> Option<&'static str> {
    match configured_backend.map(str::trim) {
        Some(value) if value.eq_ignore_ascii_case(TINY_SKIA_BACKEND) || value == "tiny_skia" => {
            Some(TINY_SKIA_BACKEND)
        }
        Some(value) if !value.is_empty() => None,
        _ if probe() => None,
        _ => Some(TINY_SKIA_BACKEND),
    }
}

fn wgpu_supported() -> bool {
    panic::catch_unwind(AssertUnwindSafe(|| {
        iced::futures::executor::block_on(probe_wgpu())
    }))
    .unwrap_or(false)
}

async fn probe_wgpu() -> bool {
    let instance = wgpu::Instance::new(&wgpu::InstanceDescriptor {
        backends: wgpu::Backends::PRIMARY,
        ..Default::default()
    });

    let adapter = match instance
        .request_adapter(&wgpu::RequestAdapterOptions {
            power_preference: wgpu::PowerPreference::HighPerformance,
            force_fallback_adapter: false,
            compatible_surface: None,
        })
        .await
    {
        Ok(adapter) => adapter,
        Err(_) => return false,
    };

    if !adapter_supported(&adapter.get_info()) {
        return false;
    }

    let limits = [
        probe_limits(wgpu::Limits::default()),
        probe_limits(wgpu::Limits::downlevel_defaults()),
    ];

    for required_limits in limits {
        let device = adapter
            .request_device(&wgpu::DeviceDescriptor {
                label: Some("filesystem-gui wgpu probe"),
                required_features: wgpu::Features::empty(),
                required_limits,
                experimental_features: wgpu::ExperimentalFeatures::disabled(),
                memory_hints: wgpu::MemoryHints::MemoryUsage,
                trace: wgpu::Trace::Off,
            })
            .await;

        let Ok((device, _queue)) = device else {
            continue;
        };

        return probe_pipeline(&device).await;
    }

    false
}

fn adapter_supported(info: &wgpu::AdapterInfo) -> bool {
    if info.device_type == wgpu::DeviceType::Cpu {
        return false;
    }

    let descriptor = format!(
        "{} {} {}",
        info.name.to_lowercase(),
        info.driver.to_lowercase(),
        info.driver_info.to_lowercase()
    );

    ![
        "llvmpipe",
        "lavapipe",
        "softpipe",
        "software rasterizer",
        "swiftshader",
    ]
    .iter()
    .any(|marker| descriptor.contains(marker))
}

fn probe_limits(limits: wgpu::Limits) -> wgpu::Limits {
    wgpu::Limits {
        max_bind_groups: 2,
        max_non_sampler_bindings: 2048,
        ..limits
    }
}

async fn probe_pipeline(device: &wgpu::Device) -> bool {
    let created = panic::catch_unwind(AssertUnwindSafe(|| {
        device.push_error_scope(wgpu::ErrorFilter::Validation);
        let shader = device.create_shader_module(wgpu::ShaderModuleDescriptor {
            label: Some("filesystem-gui wgpu probe shader"),
            source: wgpu::ShaderSource::Wgsl(Cow::Borrowed(WGPU_PROBE_SHADER)),
        });

        let pipeline_layout = device.create_pipeline_layout(&wgpu::PipelineLayoutDescriptor {
            label: Some("filesystem-gui wgpu probe pipeline layout"),
            bind_group_layouts: &[],
            push_constant_ranges: &[],
        });

        let _pipeline = device.create_render_pipeline(&wgpu::RenderPipelineDescriptor {
            label: Some("filesystem-gui wgpu probe pipeline"),
            layout: Some(&pipeline_layout),
            vertex: wgpu::VertexState {
                module: &shader,
                entry_point: Some("vs_main"),
                buffers: &[],
                compilation_options: wgpu::PipelineCompilationOptions::default(),
            },
            primitive: wgpu::PrimitiveState::default(),
            depth_stencil: None,
            multisample: wgpu::MultisampleState::default(),
            fragment: Some(wgpu::FragmentState {
                module: &shader,
                entry_point: Some("fs_main"),
                targets: &[Some(wgpu::ColorTargetState {
                    format: wgpu::TextureFormat::Rgba8Unorm,
                    blend: Some(wgpu::BlendState::REPLACE),
                    write_mask: wgpu::ColorWrites::ALL,
                })],
                compilation_options: wgpu::PipelineCompilationOptions::default(),
            }),
            multiview: None,
            cache: None,
        });
    }));

    if created.is_err() {
        return false;
    }

    device.pop_error_scope().await.is_none()
}

#[cfg(test)]
mod tests {
    use super::{adapter_supported, selected_backend, wgpu, TINY_SKIA_BACKEND};

    fn adapter_info(name: &str, device_type: wgpu::DeviceType) -> wgpu::AdapterInfo {
        wgpu::AdapterInfo {
            name: name.to_string(),
            vendor: 0,
            device: 0,
            device_type,
            driver: String::new(),
            driver_info: String::new(),
            backend: wgpu::Backend::Vulkan,
        }
    }

    #[test]
    fn keeps_explicit_wgpu_backend() {
        assert_eq!(selected_backend(Some("wgpu"), || false), None);
    }

    #[test]
    fn keeps_explicit_backend_list() {
        assert_eq!(selected_backend(Some("wgpu,tiny-skia"), || false), None);
    }

    #[test]
    fn normalizes_explicit_tiny_skia_backend() {
        assert_eq!(
            selected_backend(Some(" tiny_skia "), || true),
            Some(TINY_SKIA_BACKEND)
        );
    }

    #[test]
    fn leaves_backend_unset_when_wgpu_probe_passes() {
        assert_eq!(selected_backend(None, || true), None);
    }

    #[test]
    fn selects_tiny_skia_when_wgpu_probe_fails() {
        assert_eq!(selected_backend(None, || false), Some(TINY_SKIA_BACKEND));
    }

    #[test]
    fn rejects_cpu_wgpu_adapters() {
        let info = adapter_info("llvmpipe (LLVM 7.0, 256 bits)", wgpu::DeviceType::Cpu);

        assert!(!adapter_supported(&info));
    }

    #[test]
    fn rejects_known_software_wgpu_adapters() {
        let mut info = adapter_info("Mesa llvmpipe", wgpu::DeviceType::Other);
        assert!(!adapter_supported(&info));

        info.name = "Mesa lavapipe".to_string();
        assert!(!adapter_supported(&info));

        info.name = "SwiftShader Device".to_string();
        assert!(!adapter_supported(&info));
    }

    #[test]
    fn accepts_hardware_wgpu_adapters() {
        let info = adapter_info("AMD Radeon RX 6600", wgpu::DeviceType::DiscreteGpu);

        assert!(adapter_supported(&info));
    }
}
