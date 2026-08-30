using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using UnityEditor;
using UnityEditor.Animations;
using UnityEditor.Formats.Fbx.Exporter;
using UnityEngine;

namespace WorldWalker.UnityExport
{
    /// <summary>
    /// Batch entry point used to turn the Wafflus Unity animation clips into FBX
    /// files that can be imported by Unreal Engine.
    /// </summary>
    public static class GenshinAnimationFbxExporter
    {
        private const string ModelPath =
            "Assets/GenshinImpactMovementSystem/Models/Characters/Player/CharacterModel.fbx";

        private const string ClipRoot =
            "Assets/GenshinImpactMovementSystem/Animations/Characters/Player/Clips";

        private const string TempRoot = "Assets/WorldWalkerExportTemp";

        [MenuItem("WorldWalker/Export Genshin Movement FBXs")]
        public static void ExportAll()
        {
            string outputDirectory = ResolveOutputDirectory();
            Directory.CreateDirectory(outputDirectory);

            GameObject modelAsset = AssetDatabase.LoadAssetAtPath<GameObject>(ModelPath);
            if (modelAsset == null)
            {
                throw new InvalidOperationException("Could not load source model: " + ModelPath);
            }

            List<string> clipPaths = AssetDatabase.FindAssets("t:AnimationClip", new[] { ClipRoot })
                .Select(AssetDatabase.GUIDToAssetPath)
                .Where(path => path.EndsWith(".anim", StringComparison.OrdinalIgnoreCase))
                .Distinct()
                .OrderBy(path => path, StringComparer.OrdinalIgnoreCase)
                .ToList();

            if (clipPaths.Count != 13)
            {
                throw new InvalidOperationException(
                    "Expected 13 external animation clips, but found " + clipPaths.Count + ".");
            }

            EnsureTempFolder();
            ExportBindPose(modelAsset, outputDirectory);

            int exportedCount = 0;
            try
            {
                foreach (string clipPath in clipPaths)
                {
                    AnimationClip clip = AssetDatabase.LoadAssetAtPath<AnimationClip>(clipPath);
                    if (clip == null)
                    {
                        throw new InvalidOperationException("Could not load clip: " + clipPath);
                    }

                    int curveCount = AnimationUtility.GetCurveBindings(clip).Length;
                    if (curveCount == 0)
                    {
                        throw new InvalidOperationException("Animation contains no transform curves: " + clipPath);
                    }

                    ExportClip(modelAsset, clip, outputDirectory);
                    exportedCount++;
                    Debug.LogFormat(
                        "WW_FBX_EXPORTED name={0} length={1:F3} curves={2}",
                        clip.name,
                        clip.length,
                        curveCount);
                }
            }
            finally
            {
                AssetDatabase.DeleteAsset(TempRoot);
                AssetDatabase.Refresh();
            }

            Debug.LogFormat(
                "WW_UNITY_ANIMATION_EXPORT_COMPLETE count={0} output={1}",
                exportedCount,
                outputDirectory);
        }

        private static void ExportBindPose(GameObject modelAsset, string outputDirectory)
        {
            GameObject instance = UnityEngine.Object.Instantiate(modelAsset);
            instance.name = "WWMixamo_Skeleton";
            try
            {
                string outputPath = Path.Combine(outputDirectory, "WWMixamo_Skeleton.fbx");
                DeleteIfPresent(outputPath);
                string result = ModelExporter.ExportObject(outputPath, instance);
                ValidateExport(result, outputPath);
            }
            finally
            {
                UnityEngine.Object.DestroyImmediate(instance);
            }
        }

        private static void ExportClip(GameObject modelAsset, AnimationClip clip, string outputDirectory)
        {
            string safeName = SanitizeFileName(clip.name);
            string controllerPath = TempRoot + "/WW_" + safeName + ".controller";
            AssetDatabase.DeleteAsset(controllerPath);

            AnimatorController controller = AnimatorController.CreateAnimatorControllerAtPath(controllerPath);
            AnimatorStateMachine stateMachine = controller.layers[0].stateMachine;
            AnimatorState state = stateMachine.AddState(clip.name);
            state.motion = clip;
            stateMachine.defaultState = state;
            EditorUtility.SetDirty(controller);
            AssetDatabase.SaveAssets();

            GameObject instance = UnityEngine.Object.Instantiate(modelAsset);
            instance.name = "WWMixamo_" + safeName;
            try
            {
                Animator animator = instance.GetComponent<Animator>();
                if (animator == null)
                {
                    animator = instance.AddComponent<Animator>();
                }

                animator.runtimeAnimatorController = controller;
                animator.applyRootMotion = false;
                animator.Rebind();
                animator.Update(0.0f);

                string outputPath = Path.Combine(outputDirectory, "WWMixamo@" + safeName + ".fbx");
                DeleteIfPresent(outputPath);
                string result = ModelExporter.ExportObject(outputPath, instance);
                ValidateExport(result, outputPath);
            }
            finally
            {
                UnityEngine.Object.DestroyImmediate(instance);
                AssetDatabase.DeleteAsset(controllerPath);
            }
        }

        private static string ResolveOutputDirectory()
        {
            string fromEnvironment = Environment.GetEnvironmentVariable("WW_UNITY_FBX_OUTPUT");
            if (!string.IsNullOrWhiteSpace(fromEnvironment))
            {
                return Path.GetFullPath(fromEnvironment);
            }

            const string argumentPrefix = "-wwOutput=";
            string argument = Environment.GetCommandLineArgs()
                .FirstOrDefault(value => value.StartsWith(argumentPrefix, StringComparison.OrdinalIgnoreCase));
            if (!string.IsNullOrEmpty(argument))
            {
                return Path.GetFullPath(argument.Substring(argumentPrefix.Length).Trim('"'));
            }

            return Path.GetFullPath(Path.Combine(Application.dataPath, "../ExportedFBX"));
        }

        private static void EnsureTempFolder()
        {
            if (!AssetDatabase.IsValidFolder(TempRoot))
            {
                AssetDatabase.CreateFolder("Assets", "WorldWalkerExportTemp");
            }
        }

        private static string SanitizeFileName(string value)
        {
            foreach (char invalidCharacter in Path.GetInvalidFileNameChars())
            {
                value = value.Replace(invalidCharacter, '_');
            }

            return value;
        }

        private static void DeleteIfPresent(string path)
        {
            if (File.Exists(path))
            {
                File.Delete(path);
            }
        }

        private static void ValidateExport(string result, string outputPath)
        {
            if (string.IsNullOrEmpty(result) || !File.Exists(outputPath))
            {
                throw new InvalidOperationException("FBX Exporter did not create: " + outputPath);
            }

            long length = new FileInfo(outputPath).Length;
            if (length < 1024)
            {
                throw new InvalidOperationException("Exported FBX is unexpectedly small: " + outputPath);
            }
        }
    }
}
