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
    Vector3 velocity;
    Color color;
    bool isStatic;
    float health;
    float maxHealth;
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

// adding a com to pushForce
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

BoundingBox GetBlockBoundingBox(Block block, Vector3 size) {
    return (BoundingBox){
        (Vector3){ block.position.x - size.x/2, 
                   block.position.y - size.y/2, 
                   block.position.z - size.z/2 },
        (Vector3){ block.position.x + size.x/2, 
                   block.position.y + size.y/2, 
                   block.position.z + size.z/2 }
    };
}

BoundingBox GetPlayerBoundingBox(Vector3 position, Vector3 size) {
    return (BoundingBox){
        (Vector3){ position.x - size.x/2, 
                   position.y - size.y, 
                   position.z - size.z/2 },
        (Vector3){ position.x + size.x/2, 
                   position.y, 
                   position.z + size.z/2 }
    };
}

Color GetHealthColor(float health, float maxHealth) {
    float healthPercent = health / maxHealth;
    if (healthPercent > 0.66f) return GREEN;
    if (healthPercent > 0.33f) return YELLOW;
    return RED;
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
    Vector3 playerSize = { 0.8f, 2.0f, 0.8f };
    float playerSpeed = 5.0f;
    float jumpForce = 8.0f;
    float gravity = 20.0f;
    bool isGrounded = false;
    float groundLevel = 0.0f;
    float playerHeight = 2.0f;
    float pushForce = 3.0f;
    float kickForce = 15.0f;
    float kickRange = 3.0f;
    float kickCooldown = 0.0f;
    
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
    Vector3 blockSize = { 2.0f, 2.0f, 2.0f };
    
    // Physics & Damage
    float friction = 0.9f;
    float blockGravity = 20.0f;
    float damageThreshold = 3.0f;  // Minimum velocity to cause damage
    float damageMultiplier = 5.0f;
    
    // Add some initial blocks with health
    blocks.push_back({ (Vector3){ -5.0f, 1.0f, 5.0f }, {0,0,0}, RED, false, 100.0f, 100.0f });
    blocks.push_back({ (Vector3){ 5.0f, 1.0f, 5.0f }, {0,0,0}, BLUE, false, 100.0f, 100.0f });
    blocks.push_back({ (Vector3){ 0.0f, 1.0f, 10.0f }, {0,0,0}, YELLOW, false, 100.0f, 100.0f });
    blocks.push_back({ (Vector3){ -10.0f, 1.0f, -5.0f }, {0,0,0}, PURPLE, true, 1000.0f, 1000.0f }); // Static - high health
    blocks.push_back({ (Vector3){ 10.0f, 1.0f, -5.0f }, {0,0,0}, ORANGE, false, 100.0f, 100.0f });
    blocks.push_back({ (Vector3){ 0.0f, 0.5f, 3.0f }, {0,0,0}, BROWN, true, 1000.0f, 1000.0f });
    
    DisableCursor();
    SetTargetFPS(60);
    
    while (!WindowShouldClose()) {
        float deltaTime = GetFrameTime();
        
        // Update kick cooldown
        if (kickCooldown > 0) kickCooldown -= deltaTime;
        
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
                
                // Movement input
                Vector3 moveDirection = { 0.0f, 0.0f, 0.0f };
                
                if (IsKeyDown(KEY_W)) {
                    moveDirection = Vector3Add(moveDirection, forward);
                }
                if (IsKeyDown(KEY_S)) {
                    moveDirection = Vector3Subtract(moveDirection, forward);
                }
                if (IsKeyDown(KEY_A)) {
                    moveDirection = Vector3Add(moveDirection, right);
                }
                if (IsKeyDown(KEY_D)) {
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
                
                // KICK ABILITY (E key)
                if (IsKeyPressed(KEY_E) && kickCooldown <= 0) {
                    kickCooldown = 0.5f; // 0.5 second cooldown
                    
                    // Find blocks in kick range
                    for (auto& block : blocks) {
                        Vector3 toBlock = Vector3Subtract(block.position, playerPosition);
                        toBlock.y = 0; // Only horizontal distance
                        float distance = Vector3Length(toBlock);
                        
                        if (distance <= kickRange && distance > 0) {
                            // Check if block is in front of player
                            Vector3 dirToBlock = Vector3Normalize(toBlock);
                            float dot = forward.x * dirToBlock.x + forward.z * dirToBlock.z;
                            
                            if (dot > 0.5f && !block.isStatic) { // In front
                                // Apply kick force
                                block.velocity.x = dirToBlock.x * kickForce;
                                block.velocity.z = dirToBlock.z * kickForce;
                                block.velocity.y = kickForce * 0.5f; // Slight upward kick
                            }
                        }
                    }
                }
                
                // Apply gravity
                if (!isGrounded) {
                    playerVelocity.y -= gravity * deltaTime;
                }
                
                // Store old position
                Vector3 oldPosition = playerPosition;
                
                // Update position
                playerPosition = Vector3Add(playerPosition, 
                    Vector3Scale(playerVelocity, deltaTime));
                
                // Get player bounding box
                BoundingBox playerBox = GetPlayerBoundingBox(playerPosition, playerSize);
                
                // Check collisions with blocks
                for (auto& block : blocks) {
                    BoundingBox blockBox = GetBlockBoundingBox(block, blockSize);
                    
                    if (CheckCollisionBoxes(playerBox, blockBox)) {
                        // Push block if not static
                        if (!block.isStatic) {
                            Vector3 pushDir = Vector3Subtract(block.position, playerPosition);
                            pushDir.y = 0;
                            
                            if (Vector3Length(pushDir) > 0) {
                                pushDir = Vector3Normalize(pushDir);
                                block.velocity.x = pushDir.x * pushForce;
                                block.velocity.z = pushDir.z * pushForce;
                            }
                        }
                        
                        // Resolve collision
                        playerPosition = oldPosition;
                        playerVelocity.x = 0;
                        playerVelocity.z = 0;
                    }
                }
                
                // Ground collision
                if (playerPosition.y <= groundLevel + playerHeight) {
                    playerPosition.y = groundLevel + playerHeight;
                    playerVelocity.y = 0.0f;
                    isGrounded = true;
                }
                
                // Update blocks physics
                for (size_t i = 0; i < blocks.size(); i++) {
                    if (!blocks[i].isStatic) {
                        // Apply friction
                        blocks[i].velocity.x *= friction;
                        blocks[i].velocity.z *= friction;
                        
                        // Apply gravity
                        blocks[i].velocity.y -= blockGravity * deltaTime;
                        
                        // Update position
                        Vector3 oldBlockPos = blocks[i].position;
                        blocks[i].position = Vector3Add(blocks[i].position, 
                            Vector3Scale(blocks[i].velocity, deltaTime));
                        
                        // Ground collision for blocks
                        if (blocks[i].position.y <= 1.0f) {
                            float impactSpeed = fabs(blocks[i].velocity.y);
                            blocks[i].position.y = 1.0f;
                            blocks[i].velocity.y = 0.0f;
                            
                            // Damage from ground impact
                            if (impactSpeed > damageThreshold) {
                                blocks[i].health -= (impactSpeed - damageThreshold) * damageMultiplier;
                            }
                        }
                        
                        // Block-to-block collision with damage
                        BoundingBox box1 = GetBlockBoundingBox(blocks[i], blockSize);
                        for (size_t j = 0; j < blocks.size(); j++) {
                            if (i != j) {
                                BoundingBox box2 = GetBlockBoundingBox(blocks[j], blockSize);
                                if (CheckCollisionBoxes(box1, box2)) {
                                    // Calculate collision velocity (impact force)
                                    Vector3 relativeVel = Vector3Subtract(blocks[i].velocity, blocks[j].velocity);
                                    float impactSpeed = Vector3Length(relativeVel);
                                    
                                    // Apply damage if collision is rough enough
                                    if (impactSpeed > damageThreshold) {
                                        float damage = (impactSpeed - damageThreshold) * damageMultiplier;
                                        blocks[i].health -= damage;
                                        if (!blocks[j].isStatic) {
                                            blocks[j].health -= damage;
                                        }
                                    }
                                    
                                    // Collision response
                                    blocks[i].position = oldBlockPos;
                                    blocks[i].velocity.x *= -0.5f;
                                    blocks[i].velocity.z *= -0.5f;
                                }
                            }
                        }
                        
                        // Stop very slow blocks
                        if (fabs(blocks[i].velocity.x) < 0.01f) blocks[i].velocity.x = 0;
                        if (fabs(blocks[i].velocity.z) < 0.01f) blocks[i].velocity.z = 0;
                    }
                }
                
                // Remove destroyed blocks
                for (int i = blocks.size() - 1; i >= 0; i--) {
                    if (blocks[i].health <= 0) {
                        blocks.erase(blocks.begin() + i);
                    }
                }
                
                // Update first-person camera
                fpCamera.position = playerPosition;
                fpCamera.target = (Vector3){
                    playerPosition.x + sinf(cameraYaw),
                    playerPosition.y + cameraPitch,
                    playerPosition.z + cosf(cameraYaw)
                };
                
            } else if (currentMode == WORLD_EDITING_MODE) {
                // World editing mode
                Vector3 moveDir = { 0.0f, 0.0f, 0.0f };
                
                if (IsKeyDown(KEY_W)) moveDir.z -= 1.0f;
                if (IsKeyDown(KEY_S)) moveDir.z += 1.0f;
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
                
                // Mouse picking
                Ray ray = GetScreenToWorldRay(GetMousePosition(), editCamera);
                float t = -ray.position.y / ray.direction.y;
                Vector3 groundPoint = {
                    ray.position.x + ray.direction.x * t,
                    0.0f,
                    ray.position.z + ray.direction.z * t
                };
                
                Vector3 snappedPos = {
                    roundf(groundPoint.x),
                    1.0f,
                    roundf(groundPoint.z)
                };
                
                // Add block
                if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                    bool blockExists = false;
                    for (const auto& block : blocks) {
                        if (Vector3Distance(block.position, snappedPos) < 0.1f) {
                            blockExists = true;
                            break;
                        }
                    }
                    
                    if (!blockExists) {
                        Color colors[] = { RED, BLUE, GREEN, YELLOW, PURPLE, ORANGE, PINK, LIME };
                        blocks.push_back({ snappedPos, {0,0,0}, colors[GetRandomValue(0, 7)], false, 100.0f, 100.0f });
                    }
                }
                
                // Remove block
                if (IsMouseButtonPressed(MOUSE_RIGHT_BUTTON)) {
                    for (int i = blocks.size() - 1; i >= 0; i--) {
                        if (Vector3Distance(blocks[i].position, snappedPos) < 0.1f) {
                            blocks.erase(blocks.begin() + i);
                            break;
                        }
                    }
                }
                
                // Toggle static
                if (IsMouseButtonPressed(MOUSE_MIDDLE_BUTTON)) {
                    for (auto& block : blocks) {
                        if (Vector3Distance(block.position, snappedPos) < 0.1f) {
                            block.isStatic = !block.isStatic;
                            if (block.isStatic) {
                                block.maxHealth = block.health = 1000.0f;
                            } else {
                                block.maxHealth = block.health = 100.0f;
                            }
                            break;
                        }
                    }
                }
            }
        }
        
        // Drawing
        BeginDrawing();
            ClearBackground(SKYBLUE);
            
            Camera3D* activeCamera = (currentMode == NORMAL_MODE) ? &fpCamera : &editCamera;
            
            BeginMode3D(*activeCamera);
                // Draw ground
                DrawPlane((Vector3){ 0.0f, 0.0f, 0.0f }, 
                    (Vector2){ 50.0f, 50.0f }, DARKGREEN);
                DrawGrid(50, 1.0f);
                
                // Draw all blocks with health indication
                for (const auto& block : blocks) {
                    Color drawColor = block.color;
                    if (block.isStatic) {
                        drawColor = Fade(block.color, 0.7f);
                    }
                    
                    DrawCube(block.position, blockSize.x, blockSize.y, blockSize.z, drawColor);
                    DrawCubeWires(block.position, blockSize.x, blockSize.y, blockSize.z, 
                        block.isStatic ? GRAY : BLACK);
                    
                    // Draw health bar above block
                    if (!block.isStatic && currentMode == NORMAL_MODE) {
                        Vector3 barPos = { block.position.x, block.position.y + 1.5f, block.position.z };
                        float healthPercent = block.health / block.maxHealth;
                        Color healthColor = GetHealthColor(block.health, block.maxHealth);
                        
                        // Background bar
                        DrawCube(barPos, 1.5f, 0.1f, 0.1f, DARKGRAY);
                        // Health bar
                        DrawCube((Vector3){barPos.x - 0.75f + (0.75f * healthPercent), barPos.y, barPos.z}, 
                                1.5f * healthPercent, 0.12f, 0.12f, healthColor);
                    }
                }
                
                // Preview in edit mode
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
                    DrawCube(previewPos, blockSize.x, blockSize.y, blockSize.z, Fade(WHITE, 0.3f));
                    DrawCubeWires(previewPos, blockSize.x, blockSize.y, blockSize.z, WHITE);
                }
                
            EndMode3D();
            
            if (!isPaused) {
                if (currentMode == NORMAL_MODE) {
                    DrawText("NORMAL MODE", 10, 10, 20, DARKGRAY);
                    DrawText("WASD - Move | SPACE - Jump | E - Kick | TAB - Pause", 10, 40, 20, DARKGRAY);
                    DrawText("Kick blocks to damage them! Blocks break on hard impacts!", 10, 70, 20, GREEN);
                    DrawText(TextFormat("Position: (%.1f, %.1f, %.1f)", 
                        playerPosition.x, playerPosition.y, playerPosition.z), 10, 100, 20, DARKGRAY);
                    
                    // Kick cooldown indicator
                    if (kickCooldown > 0) {
                        DrawText(TextFormat("Kick Cooldown: %.1fs", kickCooldown), 10, 130, 20, RED);
                    } else {
                        DrawText("Kick Ready!", 10, 130, 20, GREEN);
                    }
                } else {
                    DrawText("WORLD EDITING MODE (W/S Inverted)", 10, 10, 25, ORANGE);
                    DrawText("WASD - Move | LMB - Add | RMB - Remove | MMB - Toggle Static", 
                        10, 40, 20, DARKGRAY);
                    DrawText(TextFormat("Blocks: %d | TAB - Pause", blocks.size()), 10, 70, 20, DARKGRAY);
                    DrawText("Faded blocks are STATIC (can't be broken)", 10, 100, 18, GRAY);
                }
                DrawFPS(10, screenHeight - 30);
                
            } else {
                // Pause menu
                DrawRectangle(0, 0, screenWidth, screenHeight, Fade(BLACK, 0.7f));
                
                const char* title = "PAUSED";
                int titleWidth = MeasureText(title, 60);
                DrawText(title, (screenWidth - titleWidth) / 2, 100, 60, WHITE);
                
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
                
                if (IsButtonClicked(normalModeBtn)) {
                    currentMode = NORMAL_MODE;
                    isPaused = false;
                    DisableCursor();
                }
                
                if (IsButtonClicked(editModeBtn)) {
                    currentMode = WORLD_EDITING_MODE;
                    isPaused = false;
                    EnableCursor();
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

