#include "raylib.h"
#include "raymath.h"
#include <vector>

// Game modes
enum GameMode {
    NORMAL_MODE,
    WORLD_EDITING_MODE
};

// Block structure
struct Block {
    Vector3 position;
    Color color;
};

// Button helper struct
struct Button {
    Rectangle bounds;
    const char* text;
    Color normalColor;
    Color hoverColor;
};

bool IsButtonHovered(Button button) {
    Vector2 mousePos = GetMousePosition();
    return CheckCollisionPointRec(mousePos, button.bounds);
}

bool IsButtonClicked(Button button) {
    return IsButtonHovered(button) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON);
}

void DrawButton(Button button) {
    Color buttonColor = IsButtonHovered(button) ? button.hoverColor : button.normalColor;
    DrawRectangleRec(button.bounds, buttonColor);
    DrawRectangleLinesEx(button.bounds, 2, BLACK);
    
    int textWidth = MeasureText(button.text, 30);
    DrawText(button.text, 
        button.bounds.x + (button.bounds.width - textWidth) / 2,
        button.bounds.y + (button.bounds.height - 30) / 2,
        30, WHITE);
}

int main() {
    // Window configuration
    const int screenWidth = 1280;
    const int screenHeight = 720;
    
    InitWindow(screenWidth, screenHeight, "3D First Person Prototype");
    
    // First-person camera
    Camera3D fpCamera = { 0 };
    fpCamera.position = (Vector3){ 0.0f, 2.0f, 0.0f };
    fpCamera.target = (Vector3){ 0.0f, 2.0f, 1.0f };
    fpCamera.up = (Vector3){ 0.0f, 1.0f, 0.0f };
    fpCamera.fovy = 60.0f;
    fpCamera.projection = CAMERA_PERSPECTIVE;
    
    // Top-down editing camera
    Camera3D editCamera = { 0 };
    editCamera.position = (Vector3){ 0.0f, 30.0f, 0.0f };
    editCamera.target = (Vector3){ 0.0f, 0.0f, 0.0f };
    editCamera.up = (Vector3){ 0.0f, 0.0f, -1.0f };
    editCamera.fovy = 45.0f;
    editCamera.projection = CAMERA_PERSPECTIVE;
    
    // Player physics variables
    Vector3 playerPosition = fpCamera.position;
    Vector3 playerVelocity = { 0.0f, 0.0f, 0.0f };
    float playerSpeed = 5.0f;
    float jumpForce = 8.0f;
    float gravity = 20.0f;
    bool isGrounded = false;
    float groundLevel = 0.0f;
    float playerHeight = 2.0f;
    
    // Mouse sensitivity
    float mouseSensitivity = 0.003f;
    float cameraYaw = 0.0f;
    float cameraPitch = 0.0f;
    
    // Game state
    bool isPaused = false;
    GameMode currentMode = NORMAL_MODE;
    
    // World editing variables
    float editCameraSpeed = 15.0f;
    Vector3 editCameraPosition = editCamera.position;
    std::vector<Block> blocks;
    
    // Add some initial blocks
    blocks.push_back({ (Vector3){ -5.0f, 1.0f, 5.0f }, RED });
    blocks.push_back({ (Vector3){ 5.0f, 1.0f, 5.0f }, BLUE });
    blocks.push_back({ (Vector3){ 0.0f, 1.0f, 10.0f }, YELLOW });
    blocks.push_back({ (Vector3){ -10.0f, 1.0f, -5.0f }, PURPLE });
    blocks.push_back({ (Vector3){ 10.0f, 1.0f, -5.0f }, ORANGE });
    blocks.push_back({ (Vector3){ 0.0f, 0.5f, 3.0f }, BROWN });
    
    DisableCursor();
    SetTargetFPS(60);
    
    while (!WindowShouldClose()) {
        float deltaTime = GetFrameTime();
        
        // Toggle pause menu with TAB
        if (IsKeyPressed(KEY_TAB)) {
            isPaused = !isPaused;
            if (isPaused) {
                EnableCursor();
            } else {
                DisableCursor();
            }
        }
        
        // Update based on mode and pause state
        if (!isPaused) {
            if (currentMode == NORMAL_MODE) {
                // First-person mode updates
                Vector2 mouseDelta = GetMouseDelta();
                cameraYaw -= mouseDelta.x * mouseSensitivity;
                cameraPitch -= mouseDelta.y * mouseSensitivity;
                
                // Clamp pitch
                if (cameraPitch > 1.5f) cameraPitch = 1.5f;
                if (cameraPitch < -1.5f) cameraPitch = -1.5f;
                
                // Calculate direction vectors
                Vector3 forward = { sinf(cameraYaw), 0.0f, cosf(cameraYaw) };
                Vector3 right = { cosf(cameraYaw), 0.0f, -sinf(cameraYaw) };
                
                // Movement input (D and A inverted)
                Vector3 moveDirection = { 0.0f, 0.0f, 0.0f };
                
                if (IsKeyDown(KEY_W)) {
                    moveDirection = Vector3Add(moveDirection, forward);
                }
                if (IsKeyDown(KEY_S)) {
                    moveDirection = Vector3Subtract(moveDirection, forward);
                }
                if (IsKeyDown(KEY_A)) {  // A moves right
                    moveDirection = Vector3Add(moveDirection, right);
                }
                if (IsKeyDown(KEY_D)) {  // D moves left
                    moveDirection = Vector3Subtract(moveDirection, right);
                }
                
                // Normalize movement
                if (Vector3Length(moveDirection) > 0) {
                    moveDirection = Vector3Normalize(moveDirection);
                }
                
                // Apply movement
                playerVelocity.x = moveDirection.x * playerSpeed;
                playerVelocity.z = moveDirection.z * playerSpeed;
                
                // Jump
                if (IsKeyPressed(KEY_SPACE) && isGrounded) {
                    playerVelocity.y = jumpForce;
                    isGrounded = false;
                }
                
                // Apply gravity
                if (!isGrounded) {
                    playerVelocity.y -= gravity * deltaTime;
                }
                
                // Update position
                playerPosition = Vector3Add(playerPosition, 
                    Vector3Scale(playerVelocity, deltaTime));
                
                // Ground collision
                if (playerPosition.y <= groundLevel + playerHeight) {
                    playerPosition.y = groundLevel + playerHeight;
                    playerVelocity.y = 0.0f;
                    isGrounded = true;
                }
                
                // Update first-person camera
                fpCamera.position = playerPosition;
                fpCamera.target = (Vector3){
                    playerPosition.x + sinf(cameraYaw),
                    playerPosition.y + cameraPitch,
                    playerPosition.z + cosf(cameraYaw)
                };
                
            } else if (currentMode == WORLD_EDITING_MODE) {
                // World editing mode - top-down camera movement
                Vector3 moveDir = { 0.0f, 0.0f, 0.0f };
                
                if (IsKeyDown(KEY_W)) moveDir.z += 1.0f;
                if (IsKeyDown(KEY_S)) moveDir.z -= 1.0f;
                if (IsKeyDown(KEY_A)) moveDir.x -= 1.0f;
                if (IsKeyDown(KEY_D)) moveDir.x += 1.0f;
                
                if (Vector3Length(moveDir) > 0) {
                    moveDir = Vector3Normalize(moveDir);
                    editCameraPosition.x += moveDir.x * editCameraSpeed * deltaTime;
                    editCameraPosition.z += moveDir.z * editCameraSpeed * deltaTime;
                }
                
                // Update editing camera
                editCamera.position = editCameraPosition;
                editCamera.target = (Vector3){ 
                    editCameraPosition.x, 
                    0.0f, 
                    editCameraPosition.z 
                };
                
                // Mouse picking for block placement/removal
                Ray ray = GetScreenToWorldRay(GetMousePosition(), editCamera);
                
                // Find intersection with ground plane (y = 0)
                float t = -ray.position.y / ray.direction.y;
                Vector3 groundPoint = {
                    ray.position.x + ray.direction.x * t,
                    0.0f,
                    ray.position.z + ray.direction.z * t
                };
                
                // Snap to grid
                Vector3 snappedPos = {
                    roundf(groundPoint.x),
                    1.0f,
                    roundf(groundPoint.z)
                };
                
                // Add block with left click
                if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                    // Check if block doesn't already exist at this position
                    bool blockExists = false;
                    for (const auto& block : blocks) {
                        if (Vector3Distance(block.position, snappedPos) < 0.1f) {
                            blockExists = true;
                            break;
                        }
                    }
                    
                    if (!blockExists) {
                        // Random color for new blocks
                        Color colors[] = { RED, BLUE, GREEN, YELLOW, PURPLE, ORANGE, PINK, LIME };
                        blocks.push_back({ snappedPos, colors[GetRandomValue(0, 7)] });
                    }
                }
                
                // Remove block with right click
                if (IsMouseButtonPressed(MOUSE_RIGHT_BUTTON)) {
                    for (int i = blocks.size() - 1; i >= 0; i--) {
                        if (Vector3Distance(blocks[i].position, snappedPos) < 0.1f) {
                            blocks.erase(blocks.begin() + i);
                            break;
                        }
                    }
                }
            }
        }
        
        // Drawing
        BeginDrawing();
            ClearBackground(SKYBLUE);
            
            // Choose camera based on mode
            Camera3D* activeCamera = (currentMode == NORMAL_MODE) ? &fpCamera : &editCamera;
            
            BeginMode3D(*activeCamera);
                // Draw ground
                DrawPlane((Vector3){ 0.0f, 0.0f, 0.0f }, 
                    (Vector2){ 50.0f, 50.0f }, DARKGREEN);
                DrawGrid(50, 1.0f);
                
                // Draw all blocks
                for (const auto& block : blocks) {
                    DrawCube(block.position, 2.0f, 2.0f, 2.0f, block.color);
                    DrawCubeWires(block.position, 2.0f, 2.0f, 2.0f, BLACK);
                }
                
                // In editing mode, show preview of block to place
                if (currentMode == WORLD_EDITING_MODE && !isPaused) {
                    Ray ray = GetScreenToWorldRay(GetMousePosition(), editCamera);
                    float t = -ray.position.y / ray.direction.y;
                    Vector3 groundPoint = {
                        ray.position.x + ray.direction.x * t,
                        0.0f,
                        ray.position.z + ray.direction.z * t
                    };
                    Vector3 previewPos = {
                        roundf(groundPoint.x),
                        1.0f,
                        roundf(groundPoint.z)
                    };
                    DrawCube(previewPos, 2.0f, 2.0f, 2.0f, Fade(WHITE, 0.3f));
                    DrawCubeWires(previewPos, 2.0f, 2.0f, 2.0f, WHITE);
                }
                
            EndMode3D();
            
            if (!isPaused) {
                // Mode-specific UI
                if (currentMode == NORMAL_MODE) {
                    DrawText("NORMAL MODE", 10, 10, 20, DARKGRAY);
                    DrawText("WASD - Move | SPACE - Jump | TAB - Pause", 10, 40, 20, DARKGRAY);
                    DrawText(TextFormat("Position: (%.1f, %.1f, %.1f)", 
                        playerPosition.x, playerPosition.y, playerPosition.z), 10, 70, 20, DARKGRAY);
                    DrawText(isGrounded ? "Grounded: YES" : "Grounded: NO", 10, 100, 20, 
                        isGrounded ? GREEN : RED);
                } else {
                    DrawText("WORLD EDITING MODE", 10, 10, 25, ORANGE);
                    DrawText("WASD - Move Camera | Left Click - Add Block | Right Click - Remove Block", 
                        10, 40, 20, DARKGRAY);
                    DrawText(TextFormat("Blocks: %d | TAB - Pause", blocks.size()), 10, 70, 20, DARKGRAY);
                }
                DrawFPS(10, screenHeight - 30);
                
            } else {
                // Pause menu
                DrawRectangle(0, 0, screenWidth, screenHeight, Fade(BLACK, 0.7f));
                
                const char* title = "PAUSED";
                int titleWidth = MeasureText(title, 60);
                DrawText(title, (screenWidth - titleWidth) / 2, 100, 60, WHITE);
                
                // Buttons
                Button normalModeBtn = {
                    (Rectangle){ screenWidth/2 - 150, 220, 300, 60 },
                    "NORMAL MODE",
                    DARKBLUE,
                    BLUE
                };
                
                Button editModeBtn = {
                    (Rectangle){ screenWidth/2 - 150, 300, 300, 60 },
                    "WORLD EDITING",
                    DARKGREEN,
                    GREEN
                };
                
                Button continueBtn = {
                    (Rectangle){ screenWidth/2 - 150, 380, 300, 60 },
                    "CONTINUE",
                    DARKPURPLE,
                    PURPLE
                };
                
                Button exitBtn = {
                    (Rectangle){ screenWidth/2 - 150, 460, 300, 60 },
                    "EXIT GAME",
                    DARKGRAY,
                    RED
                };
                
                DrawButton(normalModeBtn);
                DrawButton(editModeBtn);
                DrawButton(continueBtn);
                DrawButton(exitBtn);
                
                // Handle clicks
                if (IsButtonClicked(normalModeBtn)) {
                    currentMode = NORMAL_MODE;
                    isPaused = false;
                    DisableCursor();
                }
                
                if (IsButtonClicked(editModeBtn)) {
                    currentMode = WORLD_EDITING_MODE;
                    isPaused = false;
                    EnableCursor();  // Keep cursor visible in edit mode
                }
                
                if (IsButtonClicked(continueBtn)) {
                    isPaused = false;
                    if (currentMode == NORMAL_MODE) {
                        DisableCursor();
                    } else {
                        EnableCursor();
                    }
                }
                
                if (IsButtonClicked(exitBtn)) {
                    break;
                }
                
                DrawText("TAB - Resume", screenWidth/2 - MeasureText("TAB - Resume", 20)/2, 
                    570, 20, LIGHTGRAY);
            }
            
        EndDrawing();
    }
    
    CloseWindow();
    return 0;
}

