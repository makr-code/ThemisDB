# frozen_string_literal: true

Gem::Specification.new do |spec|
  spec.name = 'themisdb'
  spec.version = '1.0.0'
  spec.authors = ['ThemisDB Contributors']
  spec.email = ['noreply@github.com']

  spec.summary = 'Official Ruby client for ThemisDB'
  spec.description = 'ThemisDB Ruby SDK - A high-performance multi-model database client with native LLM integration'
  spec.homepage = 'https://github.com/makr-code/ThemisDB'
  spec.license = 'MIT'
  spec.required_ruby_version = '>= 2.7.0'

  spec.metadata = {
    'bug_tracker_uri' => 'https://github.com/makr-code/ThemisDB/issues',
    'changelog_uri' => 'https://github.com/makr-code/ThemisDB/blob/main/CHANGELOG.md',
    'documentation_uri' => 'https://makr-code.github.io/ThemisDB/',
    'homepage_uri' => 'https://github.com/makr-code/ThemisDB',
    'source_code_uri' => 'https://github.com/makr-code/ThemisDB'
  }

  spec.files = Dir['lib/**/*', 'LICENSE', 'README.md']
  spec.require_paths = ['lib']

  spec.add_development_dependency 'rspec', '~> 3.12'
  spec.add_development_dependency 'rubocop', '~> 1.50'
  spec.add_development_dependency 'webmock', '~> 3.18'
end
