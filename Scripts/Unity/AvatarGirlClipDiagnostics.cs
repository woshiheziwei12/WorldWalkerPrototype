using System;
using System.Collections.Generic;
using System.Linq;
using UnityEditor;
using UnityEngine;
using UnityEngine.Animations;
using UnityEngine.Playables;

namespace AvatarGirlOriginalExport
{
    public static class AvatarGirlClipDiagnostics
    {
        private const string ModelPath =
            "Assets/DirectPlayerGirl/Avatar_Girl_Sword_PlayerGirl_HeroEntity.fbx";

        private static readonly string[] ClipPaths =
        {
            "Assets/AvatarGirlGenericFixedSource/Ani_Avatar_Girl_Standby.anim",
            "Assets/AvatarGirlGenericFixedSource/Ani_Avatar_Girl_WalkCycle.anim",
            "Assets/AvatarGirlGenericFixedSource/Ani_Avatar_Girl_RunCycle.anim",
            "Assets/AvatarGirlGenericFixedSource/Ani_Avatar_Girl_RunBS.anim",
            "Assets/AvatarGirlGenericFixedSource/Ani_Avatar_Girl_SprintCycle.anim",
            "Assets/AvatarGirlGenericFixedSource/Ani_Avatar_Girl_SprintBS.anim",
            "Assets/AvatarGirlGenericFixedSource/Ani_Avatar_Girl_Sword_WeaponStandby.anim"
        };

        private static readonly string[] DiagnosticBones =
        {
            "Bip001 L Thigh", "Bip001 R Thigh",
            "Bip001 L Calf", "Bip001 R Calf",
            "Bip001 L UpperArm", "Bip001 R UpperArm"
        };

        public static void Run()
        {
            GameObject modelAsset = AssetDatabase.LoadAssetAtPath<GameObject>(ModelPath);
            if (modelAsset == null)
            {
                throw new InvalidOperationException("Missing PlayerGirl model: " + ModelPath);
            }

            foreach (string clipPath in ClipPaths)
            {
                AnimationClip clip = AssetDatabase.LoadAssetAtPath<AnimationClip>(clipPath);
                if (clip == null)
                {
                    Debug.LogWarning("AVATARGIRL_DIAG_MISSING " + clipPath);
                    continue;
                }
                DiagnoseClip(modelAsset, clip, clipPath);
            }
        }

        private static void DiagnoseClip(GameObject modelAsset, AnimationClip clip, string clipPath)
        {
            GameObject instance = UnityEngine.Object.Instantiate(modelAsset);
            instance.name = "AvatarGirlDiagnostic";
            try
            {
                Animator animator = instance.GetComponentInChildren<Animator>(true);
                if (animator == null)
                {
                    animator = instance.AddComponent<Animator>();
                }
                Avatar avatar = AssetDatabase.LoadAllAssetsAtPath(ModelPath)
                    .OfType<Avatar>()
                    .FirstOrDefault();
                if (avatar == null || !avatar.isValid || !avatar.isHuman)
                {
                    throw new InvalidOperationException("PlayerGirl Humanoid Avatar is missing or invalid.");
                }
                animator.avatar = avatar;
                Transform[] transforms = instance.GetComponentsInChildren<Transform>(true);
                Dictionary<string, Transform> bones = DiagnosticBones.ToDictionary(
                    name => name,
                    name => transforms.FirstOrDefault(transform => transform.name == name),
                    StringComparer.Ordinal);
                if (bones.Values.Any(value => value == null))
                {
                    throw new InvalidOperationException("Diagnostic bones missing from PlayerGirl model.");
                }

                Dictionary<string, Quaternion> first = new Dictionary<string, Quaternion>();
                Dictionary<string, float> maximum = DiagnosticBones.ToDictionary(
                    name => name,
                    _ => 0.0f,
                    StringComparer.Ordinal);

                PlayableGraph graph = PlayableGraph.Create("AvatarGirlClipDiagnostic_" + clip.name);
                graph.SetTimeUpdateMode(DirectorUpdateMode.Manual);
                AnimationPlayableOutput output = AnimationPlayableOutput.Create(
                    graph,
                    "Output",
                    animator);
                AnimationClipPlayable playable = AnimationClipPlayable.Create(graph, clip);
                playable.SetApplyFootIK(false);
                playable.SetApplyPlayableIK(false);
                output.SetSourcePlayable(playable);
                graph.Play();
                try
                {
                    int frameCount = Mathf.Max(1, Mathf.CeilToInt(clip.length * 60.0f));
                    for (int frame = 0; frame <= frameCount; ++frame)
                    {
                        float time = Mathf.Min(clip.length, frame / 60.0f);
                        playable.SetTime(time);
                        graph.Evaluate(0.0f);
                        foreach (KeyValuePair<string, Transform> pair in bones)
                        {
                            Quaternion rotation = pair.Value.localRotation;
                            if (frame == 0)
                            {
                                first[pair.Key] = rotation;
                            }
                            maximum[pair.Key] = Mathf.Max(
                                maximum[pair.Key],
                                Quaternion.Angle(first[pair.Key], rotation));
                        }
                    }
                }
                finally
                {
                    graph.Destroy();
                }

                EditorCurveBinding[] curveBindings = AnimationUtility.GetCurveBindings(clip);
                string topCurves = string.Join(",", curveBindings
                    .Select(binding =>
                    {
                        AnimationCurve curve = AnimationUtility.GetEditorCurve(clip, binding);
                        float minimum = curve == null || curve.length == 0
                            ? 0.0f
                            : curve.keys.Min(key => key.value);
                        float maximumValue = curve == null || curve.length == 0
                            ? 0.0f
                            : curve.keys.Max(key => key.value);
                        return new
                        {
                            Binding = binding,
                            Range = maximumValue - minimum,
                            Minimum = minimum,
                            Maximum = maximumValue
                        };
                    })
                    .OrderByDescending(item => item.Range)
                    .Take(16)
                    .Select(item => string.Format(
                        "{0}|{1}|{2:F3}:{3:F3}",
                        item.Binding.path,
                        item.Binding.propertyName,
                        item.Minimum,
                        item.Maximum)));
                string angles = string.Join(",", maximum.Select(
                    pair => pair.Key + "=" + pair.Value.ToString("F3")));
                Debug.LogFormat(
                    "AVATARGIRL_DIAG clip={0} path={1} length={2:F3} human={3} " +
                    "curveCount={4} angles=[{5}] topCurves=[{6}]",
                    clip.name,
                    clipPath,
                    clip.length,
                    clip.isHumanMotion,
                    curveBindings.Length,
                    angles,
                    topCurves);
            }
            finally
            {
                UnityEngine.Object.DestroyImmediate(instance);
            }
        }
    }
}
