# Contributing to Wireless POV POI

Thank you for your interest in contributing to the Wireless POV POI project! This document provides guidelines for contributions.

## Ways to Contribute

### 1. Report Bugs
- Use the issue tracker
- Describe the problem clearly
- Include steps to reproduce
- Provide serial monitor output
- Share hardware setup details

### 2. Suggest Features
- Open an issue with "Feature Request" label
- Describe the feature and use case
- Explain why it would be useful
- Consider implementation complexity

### 3. Submit Code
- Fork the repository
- Create a feature branch
- Make your changes
- Test thoroughly
- Submit a pull request

### 4. Improve Documentation
- Fix typos or unclear instructions
- Add examples or tutorials
- Translate to other languages
- Share photos of your build

### 5. Share Your Designs
- Share POV images you've created
- Submit custom patterns
- Share physical designs (3D models, etc.)
- Post videos of your POI in action

## Code Guidelines

### General Principles
- Keep changes minimal and focused
- Follow existing code style
- Comment complex logic
- Test before submitting
- Update documentation as needed

### Teensy Firmware
```cpp
// Style guidelines
- Use descriptive variable names
- Keep functions focused and short
- Add comments for non-obvious code
- Use FastLED library efficiently
- Avoid blocking operations
```

### ESP32 Firmware
```cpp
// Style guidelines
- Handle errors gracefully
- Validate API inputs
- Keep response times fast
- Use async where appropriate
- Document API changes
```

### Web Interface
```javascript
// Style guidelines
- Use modern JavaScript
- Keep UI responsive
- Test on mobile devices
- Minimize dependencies
- Follow accessibility best practices
```

## Pull Request Process

1. **Fork and Clone**
   ```bash
   git clone https://github.com/YOUR_USERNAME/wireless-pov-poi.git
   cd wireless-pov-poi
   ```

2. **Create Branch**
   ```bash
   git checkout -b feature/your-feature-name
   ```

3. **Make Changes**
   - Edit code
   - Test thoroughly
   - Update documentation

4. **Commit Changes**
   ```bash
   git add .
   git commit -m "Add: brief description of change"
   ```

5. **Push to GitHub**
   ```bash
   git push origin feature/your-feature-name
   ```

6. **Create Pull Request**
   - Go to GitHub
   - Click "New Pull Request"
   - Describe your changes
   - Reference related issues

## Testing Requirements

Before submitting:

### Hardware Testing
- [ ] Test on actual hardware
- [ ] Verify all features work
- [ ] Check power consumption
- [ ] Test edge cases
- [ ] Verify no regressions

### Software Testing
- [ ] Code compiles without errors
- [ ] Code compiles without warnings
- [ ] All modes tested
- [ ] Web interface tested
- [ ] API endpoints tested

### Documentation Testing
- [ ] Instructions are clear
- [ ] Links work
- [ ] Code examples compile
- [ ] No typos or errors

## Code Review Process

All submissions require review:

1. Automated checks run first
2. Maintainer reviews code
3. Feedback provided
4. Changes requested if needed
5. Once approved, merged

## Commit Message Format

Use clear, descriptive commit messages:

```
Type: Brief description (50 chars max)

Detailed explanation if needed (wrap at 72 chars)

- Bullet points for multiple changes
- Reference issues: Fixes #123
```

**Types:**
- `Add:` New feature
- `Fix:` Bug fix
- `Update:` Modify existing feature
- `Docs:` Documentation only
- `Style:` Code style changes
- `Refactor:` Code restructuring
- `Test:` Add or modify tests
- `Chore:` Maintenance tasks

## Feature Development

### Before Starting
1. Check if feature already exists
2. Search for related issues
3. Discuss approach in issue
4. Get feedback from maintainers

### During Development
1. Keep changes focused
2. Test incrementally
3. Document as you go
4. Commit logical chunks

### After Completion
1. Final testing
2. Update documentation
3. Add examples if applicable
4. Submit pull request

## Bug Fix Process

### Reporting
1. Check if already reported
2. Create detailed issue
3. Include reproduction steps
4. Share relevant code/logs

### Fixing
1. Reference issue in branch name
2. Add test case if possible
3. Fix the bug
4. Verify fix works
5. Submit pull request

## Documentation Standards

### Code Comments
```cpp
/**
 * Brief function description
 * 
 * @param paramName Parameter description
 * @return Return value description
 */
```

### README Updates
- Keep formatting consistent
- Use clear headings
- Include examples
- Link to related docs

### API Documentation
- Document all endpoints
- Include examples
- Show request/response
- Note breaking changes

## Community Guidelines

### Be Respectful
- Welcome newcomers
- Be patient with questions
- Provide constructive feedback
- Assume good intentions

### Be Helpful
- Answer questions
- Share knowledge
- Provide examples
- Link to resources

### Be Professional
- Stay on topic
- Keep discussions civil
- Respect privacy
- Follow code of conduct

## Getting Help

Need help contributing?

- Check existing documentation
- Review closed pull requests
- Ask in issue tracker
- Be specific about your question

## Recognition

Contributors will be:
- Listed in CONTRIBUTORS file
- Credited in release notes
- Acknowledged in documentation
- Appreciated by the community!

## License

By contributing, you agree that your contributions will be licensed under the MIT License.

## Questions?

If you have questions about contributing:
1. Check the documentation
2. Search existing issues
3. Open a new issue
4. Ask the community

---

**Thank you for contributing to Wireless POV POI!** Every contribution, no matter how small, helps make the project better. 🎨✨
