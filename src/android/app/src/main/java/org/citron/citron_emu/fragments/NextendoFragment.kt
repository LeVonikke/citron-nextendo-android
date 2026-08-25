// SPDX-FileCopyrightText: Copyright 2026 citron Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

package org.citron.citron_emu.fragments

import android.os.Bundle
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.Toast
import androidx.core.view.isVisible
import androidx.fragment.app.Fragment
import androidx.fragment.app.activityViewModels
import androidx.lifecycle.lifecycleScope
import androidx.navigation.findNavController
import androidx.recyclerview.widget.LinearLayoutManager
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext
import org.citron.citron_emu.R
import org.citron.citron_emu.adapters.NextendoFriendAdapter
import org.citron.citron_emu.databinding.FragmentNextendoBinding
import org.citron.citron_emu.model.HomeViewModel
import org.citron.citron_emu.model.NextendoFriend
import org.citron.citron_emu.utils.NativeNextendo

class NextendoFragment : Fragment() {
    private var _binding: FragmentNextendoBinding? = null
    private val binding get() = _binding!!

    private val homeViewModel: HomeViewModel by activityViewModels()

    private lateinit var adapter: NextendoFriendAdapter

    override fun onCreateView(
        inflater: LayoutInflater,
        container: ViewGroup?,
        savedInstanceState: Bundle?
    ): View {
        _binding = FragmentNextendoBinding.inflate(layoutInflater)
        return binding.root
    }

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        homeViewModel.setNavigationVisibility(visible = false, animated = true)
        homeViewModel.setStatusBarShadeVisibility(visible = false)

        binding.toolbarNextendo.setNavigationOnClickListener {
            binding.root.findNavController().popBackStack()
        }

        adapter = NextendoFriendAdapter(
            emptyList(),
            onAccept = { runFriendAction { NativeNextendo.acceptFriend(it.pid) } },
            onDecline = { runFriendAction { NativeNextendo.declineFriend(it.pid) } },
            onRemove = { runFriendAction { NativeNextendo.removeFriend(it.pid) } }
        )
        binding.nextendoFriendsList.apply {
            layoutManager = LinearLayoutManager(requireContext())
            adapter = this@NextendoFragment.adapter
        }
        binding.nextendoSwipeRefresh.setOnRefreshListener { refreshFriends() }

        binding.nextendoAddFriendButton.setOnClickListener { addFriend() }

        refreshAccountHeader()
    }

    override fun onResume() {
        super.onResume()
        refreshAccountHeader()
    }

    override fun onDestroyView() {
        super.onDestroyView()
        _binding = null
    }

    private fun refreshAccountHeader() {
        val linked = NativeNextendo.isLinked()
        if (linked) {
            binding.nextendoAccountStatus.text =
                getString(R.string.nextendo_signed_in_as, NativeNextendo.getUsername())
            binding.nextendoSignInOutButton.setText(R.string.nextendo_sign_out)
            binding.nextendoSignInOutButton.setOnClickListener { signOut() }
            binding.nextendoAddFriendRow.isVisible = true
            refreshFriends()
        } else {
            binding.nextendoAccountStatus.setText(R.string.nextendo_not_signed_in)
            binding.nextendoSignInOutButton.setText(R.string.nextendo_sign_in)
            binding.nextendoSignInOutButton.setOnClickListener { signIn() }
            binding.nextendoAddFriendRow.isVisible = false
            adapter.replaceList(emptyList())
            binding.nextendoEmptyState.isVisible = false
        }
    }

    private fun signIn() {
        ProgressDialogFragment.newInstance(
            requireActivity(),
            R.string.nextendo_signing_in,
            false
        ) { _, _ ->
            val error = NativeNextendo.signIn()
            if (error.isEmpty()) {
                getString(R.string.nextendo_signed_in_as, NativeNextendo.getUsername())
            } else {
                error
            }
        }.show(parentFragmentManager, ProgressDialogFragment.TAG)
        // refreshAccountHeader() runs again in onResume once the dialog is dismissed.
    }

    private fun signOut() {
        NativeNextendo.signOut()
        refreshAccountHeader()
    }

    private fun addFriend() {
        val code = binding.nextendoFriendCodeInput.text?.toString()?.trim().orEmpty()
        if (code.isEmpty()) {
            return
        }
        viewLifecycleOwner.lifecycleScope.launch {
            val error = withContext(Dispatchers.IO) { NativeNextendo.addFriendByCode(code) }
            if (error.isEmpty()) {
                binding.nextendoFriendCodeInput.setText("")
                Toast.makeText(
                    requireContext(),
                    getString(R.string.nextendo_request_sent, code),
                    Toast.LENGTH_SHORT
                ).show()
            } else {
                Toast.makeText(requireContext(), error, Toast.LENGTH_LONG).show()
            }
            refreshFriends()
        }
    }

    private fun runFriendAction(action: () -> String) {
        viewLifecycleOwner.lifecycleScope.launch {
            val error = withContext(Dispatchers.IO) { action() }
            if (error.isNotEmpty()) {
                Toast.makeText(requireContext(), error, Toast.LENGTH_LONG).show()
            }
            refreshFriends()
        }
    }

    private fun refreshFriends() {
        viewLifecycleOwner.lifecycleScope.launch {
            val friends = withContext(Dispatchers.IO) { NativeNextendo.getFriends() }
            binding.nextendoSwipeRefresh.isRefreshing = false
            adapter.replaceList(sortFriends(friends))
            binding.nextendoEmptyState.isVisible = friends.isEmpty()
        }
    }

    private fun sortFriends(friends: Array<NextendoFriend>): List<NextendoFriend> =
        friends.sortedWith(
            compareByDescending<NextendoFriend> { it.isIncomingRequest }
                .thenByDescending { it.presenceStatus != NextendoFriend.PRESENCE_OFFLINE }
                .thenBy { it.name }
        )
}
